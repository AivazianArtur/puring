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
PIPE_CAPACITY = 4 * 1024 * 1024    # bump the default 64 KiB pipe buffer
STRIPES_PER_FILE = 8               # concurrent pipe pairs per file (queue depth)

ROOT = "docs/benchmark_splice"
SRC = os.path.join(ROOT, "src")
DST = os.path.join(ROOT, "dst")

SRC_FILES = [os.path.join(SRC, f"{i}.bin") for i in range(NR_FILES)]
DST_FILES = [os.path.join(DST, f"{i}.bin") for i in range(NR_FILES)]

INIT_BLOCK = os.urandom(CHUNK)

# Linux-only fcntl ops, not exposed as named constants in the fcntl module.
F_SETPIPE_SZ = 1031
F_GETPIPE_SZ = 1032


def throughput(seconds):
    total = NR_FILES * FILE_SIZE
    return total / seconds / 1024 / 1024


def physical_bytes(path):
    """Actual on-disk allocation, in bytes - st_blocks is always in
    512-byte units regardless of the filesystem's real block size."""
    return os.stat(path).st_blocks * 512


def detect_reflink(dst_files, expected_size, threshold=0.5):
    """Best-effort detection of a CoW clone (btrfs/xfs reflink) instead
    of a real data copy: if a copy_file_range() result occupies far
    fewer physical blocks than the source, os.copy_file_range almost
    certainly cloned extents rather than moving bytes - the timer isn't
    measuring I/O throughput at all, just metadata-op latency.

    INIT_BLOCK is os.urandom(...), so it doesn't compress away under
    filesystems mounted with transparent compression (e.g. btrfs
    compress=zstd) - a real copy of incompressible data should still
    land close to expected_size on disk.

    Returns True if ANY destination file looks reflinked - one clone
    is enough to invalidate the whole run as a throughput measurement.
    """
    for path in dst_files:
        if physical_bytes(path) < expected_size * threshold:
            return True
    return False


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


def make_pipe():
    """Create a pipe and try to grow its buffer to PIPE_CAPACITY. Returns
    (pipe_r, pipe_w, actual_size) - actual_size lets callers notice a
    silently-capped pipe (common when /proc/sys/fs/pipe-max-size is low,
    e.g. inside some containers/VMs)."""
    pipe_r, pipe_w = os.pipe()
    try:
        fcntl.fcntl(pipe_w, F_SETPIPE_SZ, PIPE_CAPACITY)
    except OSError:
        pass  # fall back to whatever the default/max is
    actual_size = fcntl.fcntl(pipe_w, F_GETPIPE_SZ)
    return pipe_r, pipe_w, actual_size


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
#    FIX vs the original version: the two legs (file->pipe, pipe->file)
#    are now run as independent concurrent coroutines (producer /
#    consumer) instead of a strict lock-step "fill the whole leg, then
#    fully drain it, then fill again". Lock-step meant the ring only
#    ever had one SQE in flight per file and paid a full Python
#    await/wakeup round trip for every single small transfer. Letting
#    producer and consumer run concurrently means io_uring can have
#    both legs in flight at once, and the pipe's own buffer provides
#    backpressure automatically (a blocking splice() naturally waits -
#    via the ring, not by blocking Python - when the pipe is full/empty).
#    PIPE_CAPACITY was also bumped from 1 MiB to 4 MiB to cut the number
#    of round trips per file by 4x on top of that.
#
#    NOTE: the exact keyword names below (src/dst/offset_src/offset_dst)
#    are taken from files.c's PuringFile_splice signature. splice() is
#    called on `anchor`, an arbitrary open PuringFile that just gives us
#    a handle onto the ring - it operates on the raw fds passed in, not
#    on `anchor` itself. Adjust names if your puring build differs.
# ---------------------------------------------------------------------

async def puring_splice_copy():
    print('Running puring splice (io_uring, pipe-relayed, pipelined, fan-out)')

    anchor = await puring.open_file(path=SRC_FILES[0])
    pipe_warning_shown = False

    async def copy_stripe(fd_in, fd_out, start_offset, stripe_len):
        """Copy one contiguous stripe of a file through its own
        producer/consumer pipe pair."""
        nonlocal pipe_warning_shown

        pipe_r, pipe_w, actual_pipe_size = make_pipe()

        if actual_pipe_size < PIPE_CAPACITY and not pipe_warning_shown:
            print(
                f'  warning: pipe capped at {actual_pipe_size} bytes '
                f'(requested {PIPE_CAPACITY}) - check '
                f'/proc/sys/fs/pipe-max-size; compensating with '
                f'{STRIPES_PER_FILE} pipes/file instead'
            )
            pipe_warning_shown = True

        end_offset = start_offset + stripe_len
        try:
            async def producer():
                offset = start_offset
                while offset < end_offset:
                    leg = min(actual_pipe_size, end_offset - offset)
                    moved = await anchor.splice(
                        src=fd_in, dst=pipe_w,
                        count=leg,
                        offset_src=offset, offset_dst=-1,
                    )
                    if moved == 0:
                        break
                    offset += moved

            async def consumer():
                offset = start_offset
                while offset < end_offset:
                    moved = await anchor.splice(
                        src=pipe_r, dst=fd_out,
                        count=actual_pipe_size,
                        offset_src=-1, offset_dst=offset,
                    )
                    if moved == 0:
                        break
                    offset += moved

            await asyncio.gather(producer(), consumer())
        finally:
            os.close(pipe_r)
            os.close(pipe_w)

    async def copy_one(src_path, dst_path):
        fd_in = os.open(src_path, os.O_RDONLY)
        fd_out = os.open(dst_path, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o644)
        try:
            # Split the file into STRIPES_PER_FILE contiguous regions,
            # each copied through its own pipe. When the kernel caps a
            # single pipe at 64 KiB (common without root - see the
            # warning above), this is what actually restores queue
            # depth: N pipes in flight beat one pipe made artificially
            # bigger, the same way multiple in-flight NVMe requests beat
            # a single big one.
            stripe_len = -(-FILE_SIZE // STRIPES_PER_FILE)  # ceil div
            stripes = []
            offset = 0
            while offset < FILE_SIZE:
                length = min(stripe_len, FILE_SIZE - offset)
                stripes.append((offset, length))
                offset += length

            await asyncio.gather(*[
                copy_stripe(fd_in, fd_out, start_offset, length)
                for start_offset, length in stripes
            ])

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
    footnotes = []

    clean_dst()
    t = sync_read_write_copy()
    results.append(('read/write baseline', t))

    clean_dst()
    t = sendfile_copy()
    results.append(('os.sendfile', t))

    clean_dst()
    t = copy_file_range_copy()
    cfr_label = 'os.copy_file_range'
    if detect_reflink(DST_FILES, FILE_SIZE):
        cfr_label = 'os.copy_file_range [*]'
        footnotes.append(
            '[*] destination files occupy far fewer physical blocks than '
            'the source - this filesystem cloned extents (CoW reflink) '
            'instead of copying data. The number above measures metadata-op '
            'latency, not I/O throughput, and is not comparable to the '
            'other rows. This is expected/likely on btrfs or xfs with '
            'reflink=1; it would not happen on ext4.'
        )
    results.append((cfr_label, t))

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
        print(f'{name:26s} {throughput(sec):10.2f} MB/s   ({sec:.2f}s)')

    for note in footnotes:
        print(f'\n{note}')


if __name__ == '__main__':
    try:
        run()
    finally:
        shutil.rmtree(ROOT, ignore_errors=True)
