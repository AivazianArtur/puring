import asyncio
import fcntl
import os
import shutil
import sys
import time

sys.path.insert(0, '')

import puring

HAVE_UVLOOP = False
try:
    import uvloop
    HAVE_UVLOOP = True
except ImportError:
    pass


# ---- Config --------------------------------------------------------------

NR_FILES = 8
FILE_SIZE = 256 * 1024 * 1024      # 256 MiB per file
CHUNK = 1024 * 1024                # chunk size for the userspace baseline
PIPE_CAPACITY = 1024 * 1024        # bump the default 64 KiB pipe buffer

ROOT = "docs/benchmark_splice"
SRC = os.path.join(ROOT, "src")
DST = os.path.join(ROOT, "dst")

SRC_FILES = [os.path.join(SRC, f"{i}.bin") for i in range(NR_FILES)]
DST_FILES = [os.path.join(DST, f"{i}.bin") for i in range(NR_FILES)]

INIT_BLOCK = os.urandom(CHUNK)

# Linux-only fcntl op, not exposed as a named constant in the fcntl module.
F_SETPIPE_SZ = 1031


def throughput(seconds):
    total = NR_FILES * FILE_SIZE
    return total / seconds / 1024 / 1024


def make_source():
    os.makedirs(SRC, exist_ok=True)
    os.makedirs(DST, exist_ok=True)
    for path in SRC_FILES:
        with open(path, 'wb') as f:
            remaining = FILE_SIZE
            while remaining:
                f.write(INIT_BLOCK)
                remaining -= CHUNK


def clean_dst():
    shutil.rmtree(DST, ignore_errors=True)
    os.makedirs(DST)


def open_pair(src, dst):
    fd_in = os.open(src, os.O_RDONLY)
    fd_out = os.open(dst, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o644)
    return fd_in, fd_out


# ---------------------------------------------------------------------
# 1. userspace copy baseline: pread into a buffer, pwrite it back out.
#    Every byte crosses the kernel/userspace boundary twice.
# ---------------------------------------------------------------------

def sync_read_write_copy():
    print('Running read/write userspace copy (baseline)')
    start = time.perf_counter()
    for src, dst in zip(SRC_FILES, DST_FILES):
        fd_in, fd_out = open_pair(src, dst)
        try:
            offset = 0
            while offset < FILE_SIZE:
                data = os.pread(fd_in, CHUNK, offset)
                os.pwrite(fd_out, data, offset)
                offset += CHUNK
            os.fsync(fd_out)
        finally:
            os.close(fd_in)
            os.close(fd_out)
    return time.perf_counter() - start


# ---------------------------------------------------------------------
# 2. os.sendfile: classic kernel-side zero-copy syscall. Since Linux
#    2.6.33 out_fd can be a regular file (not just a socket), so this
#    works directly for file-to-file copies with no pipe needed.
# ---------------------------------------------------------------------

def sendfile_copy():
    print('Running os.sendfile (kernel zero-copy)')
    start = time.perf_counter()
    for src, dst in zip(SRC_FILES, DST_FILES):
        fd_in, fd_out = open_pair(src, dst)
        try:
            offset = 0
            while offset < FILE_SIZE:
                sent = os.sendfile(fd_out, fd_in, offset, FILE_SIZE - offset)
                if sent == 0:
                    break
                offset += sent
            os.fsync(fd_out)
        finally:
            os.close(fd_in)
            os.close(fd_out)
    return time.perf_counter() - start


# ---------------------------------------------------------------------
# 3. os.copy_file_range: the modern zero-copy API (Linux 4.5+). On
#    filesystems with reflink support (btrfs, xfs...) this can become
#    a metadata-only operation; on ext4 it still skips the userspace
#    round-trip.
# ---------------------------------------------------------------------

def copy_file_range_copy():
    print('Running os.copy_file_range')
    start = time.perf_counter()
    for src, dst in zip(SRC_FILES, DST_FILES):
        fd_in, fd_out = open_pair(src, dst)
        try:
            offset = 0
            while offset < FILE_SIZE:
                n = os.copy_file_range(
                    fd_in, fd_out, FILE_SIZE - offset,
                    offset_src=offset, offset_dst=offset,
                )
                if n == 0:
                    break
                offset += n
            os.fsync(fd_out)
        finally:
            os.close(fd_in)
            os.close(fd_out)
    return time.perf_counter() - start


# ---------------------------------------------------------------------
# 4. asyncio threadpool + os.sendfile, one task per file, concurrent.
# ---------------------------------------------------------------------

def _sendfile_worker(src, dst):
    fd_in, fd_out = open_pair(src, dst)
    try:
        offset = 0
        while offset < FILE_SIZE:
            sent = os.sendfile(fd_out, fd_in, offset, FILE_SIZE - offset)
            if sent == 0:
                break
            offset += sent
        os.fsync(fd_out)
    finally:
        os.close(fd_in)
        os.close(fd_out)


async def asyncio_sendfile_copy():
    print('Running asyncio threadpool + os.sendfile')
    start = time.perf_counter()
    await asyncio.gather(*[
        asyncio.to_thread(_sendfile_worker, src, dst)
        for src, dst in zip(SRC_FILES, DST_FILES)
    ])
    return time.perf_counter() - start


async def uvloop_sendfile_copy():
    print('Running uvloop threadpool + os.sendfile')
    start = time.perf_counter()
    await asyncio.gather(*[
        asyncio.to_thread(_sendfile_worker, src, dst)
        for src, dst in zip(SRC_FILES, DST_FILES)
    ])
    return time.perf_counter() - start


# ---------------------------------------------------------------------
# 5. puring splice: IORING_OP_SPLICE submitted from the single-threaded
#    ring, no worker threads involved at all.
#
#    splice(2) requires one end of the transfer to be a pipe, so even a
#    "direct" file-to-file zero-copy still needs a pipe as relay:
#    file_in -> pipe -> file_out. Both legs stay entirely in kernel
#    space; the data is never copied into a userspace buffer.
#
#    NOTE: the exact keyword names below (src/dst/offset_src/offset_dst)
#    are taken from files.c's PuringFile_splice signature. splice() is
#    called on `anchor`, an arbitrary open PuringFile that just gives us
#    a handle onto the ring — it operates on the raw fds passed in, not
#    on `anchor` itself. Adjust names if your puring build differs.
# ---------------------------------------------------------------------

async def puring_splice_copy():
    print('Running puring splice (io_uring, pipe-relayed)')

    anchor = await puring.open_file(path=SRC_FILES[0])

    async def copy_one(src_path, dst_path):
        fd_in = os.open(src_path, os.O_RDONLY)
        fd_out = os.open(dst_path, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o644)
        pipe_r, pipe_w = os.pipe()

        try:
            fcntl.fcntl(pipe_w, F_SETPIPE_SZ, PIPE_CAPACITY)
        except OSError:
            pass  # fall back to the default 64 KiB pipe buffer

        try:
            offset = 0
            while offset < FILE_SIZE:
                leg = min(PIPE_CAPACITY, FILE_SIZE - offset)

                # file -> pipe (offset_dst=-1: pipes aren't seekable)
                moved_in = await anchor.splice(
                    src=fd_in, dst=pipe_w,
                    count=leg,
                    offset_src=offset, offset_dst=-1,
                )

                # pipe -> file, may take more than one splice to drain
                remaining = moved_in
                while remaining:
                    moved_out = await anchor.splice(
                        src=pipe_r, dst=fd_out,
                        count=remaining,
                        offset_src=-1, offset_dst=offset,
                    )
                    offset += moved_out
                    remaining -= moved_out

            # splice only queues writes into the page cache; without an
            # explicit fsync the kernel is free to write it back whenever
            # it feels like it, and our timer would stop long before the
            # data is actually durable on the NVMe. Match the other
            # variants, which all fsync before their timer stops.
            #
            # NOTE: this is a blocking call, not asyncio.to_thread(). The
            # PuringLoop only wakes up on io_uring completions; it has no
            # wakeup path for a ThreadPoolExecutor callback landing from
            # another thread (no self-pipe/eventfd for that), so
            # to_thread() here just hangs forever waiting for a wakeup
            # that never comes. A short blocking fsync is safe and simple.
            os.fsync(fd_out)
        finally:
            os.close(pipe_r)
            os.close(pipe_w)
            os.close(fd_in)
            os.close(fd_out)

    start = time.perf_counter()
    await asyncio.gather(*[
        copy_one(src, dst)
        for src, dst in zip(SRC_FILES, DST_FILES)
    ])
    elapsed = time.perf_counter() - start

    await anchor.close()
    return elapsed


def run():
    total_gb = NR_FILES * FILE_SIZE / 1024 ** 3
    print(f'{NR_FILES=}, {FILE_SIZE=}, total={total_gb:.2f} GiB')

    make_source()
    results = []

    clean_dst()
    t = sync_read_write_copy()
    results.append(('read/write baseline', t))

    clean_dst()
    t = sendfile_copy()
    results.append(('os.sendfile', t))

    clean_dst()
    t = copy_file_range_copy()
    results.append(('os.copy_file_range', t))

    clean_dst()
    t = asyncio.run(asyncio_sendfile_copy())
    results.append(('asyncio + sendfile', t))

    if HAVE_UVLOOP:
        uvloop.install()
        clean_dst()
        t = asyncio.run(uvloop_sendfile_copy())
        results.append(('uvloop + sendfile', t))

    clean_dst()
    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        t = runner.run(puring_splice_copy())
    results.append(('puring splice', t))

    print('\n==== RESULTS ====\n')
    for name, sec in results:
        print(f'{name:22s} {throughput(sec):10.2f} MB/s   ({sec:.2f}s)')


if __name__ == '__main__':
    try:
        run()
    finally:
        shutil.rmtree(ROOT, ignore_errors=True)
