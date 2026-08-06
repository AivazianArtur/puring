import asyncio
import os
import shutil
import sys
import time

is_uvloop_installed = False
try:
    import uvloop
    is_uvloop_installed = True
except ImportError:
    pass

sys.path.insert(0, '')

import puring


# ---- Config ---------------------------------------------------------------
# Each "iteration" reads NR_CHUNKS * CHUNK_SIZE contiguous bytes from the
# source file, either as one vectored readv() into NR_CHUNKS buffers, or as
# NR_CHUNKS separate scalar reads. The point isn't disk throughput (the
# file is small enough to sit in the page cache after the first pass) -
# it's how much submission/syscall overhead a scatter-gather read saves
# you versus firing one read per buffer.

CHUNK_SIZE = 256 * 1024
NR_CHUNKS = 32
ITERATIONS = 200

TOTAL_SIZE = CHUNK_SIZE * NR_CHUNKS * ITERATIONS

FILES_FOLDER = 'docs/benchmark_readv/'
SRC = FILES_FOLDER + 'source.bin'


def mbps(seconds):
    return TOTAL_SIZE / seconds / (1024 ** 2)


def make_source():
    print(f'Preparing {TOTAL_SIZE / (1024 ** 2):.0f} MiB source file')
    block = os.urandom(CHUNK_SIZE)
    with open(SRC, 'wb') as f:
        for _ in range(NR_CHUNKS * ITERATIONS):
            f.write(block)


def offsets():
    step = CHUNK_SIZE * NR_CHUNKS
    return [i * step for i in range(ITERATIONS)]


# ---------------------------------------------------------------------
# 1. sync baseline: one os.readv() syscall per iteration, NR_CHUNKS
#    buffers filled in a single call.
# ---------------------------------------------------------------------

def sync_readv():
    print('Running sync os.readv (one syscall per iteration)')
    fd = os.open(SRC, os.O_RDONLY)
    buffers = [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]

    start = time.perf_counter()
    for off in offsets():
        os.lseek(fd, off, os.SEEK_SET)
        os.readv(fd, buffers)
    elapsed = time.perf_counter() - start

    os.close(fd)
    return elapsed


# ---------------------------------------------------------------------
# 2. sync scalar baseline: NR_CHUNKS separate os.pread() calls per
#    iteration instead of one readv().
# ---------------------------------------------------------------------

def sync_scalar_read():
    print('Running sync os.pread scalar (NR_CHUNKS syscalls per iteration)')
    fd = os.open(SRC, os.O_RDONLY)

    start = time.perf_counter()
    for base in offsets():
        for i in range(NR_CHUNKS):
            os.pread(fd, CHUNK_SIZE, base + i * CHUNK_SIZE)
    elapsed = time.perf_counter() - start

    os.close(fd)
    return elapsed


# ---------------------------------------------------------------------
# 3. asyncio threadpool: readv still issued as a single blocking call
#    per iteration, just off the event loop thread.
# ---------------------------------------------------------------------

async def asyncio_readv():
    print('Running asyncio threadpool os.readv')

    def worker():
        fd = os.open(SRC, os.O_RDONLY)
        buffers = [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]
        for off in offsets():
            os.lseek(fd, off, os.SEEK_SET)
            os.readv(fd, buffers)
        os.close(fd)

    start = time.perf_counter()
    await asyncio.to_thread(worker)
    return time.perf_counter() - start


# ---------------------------------------------------------------------
# 4. asyncio threadpool, scalar: each of the NR_CHUNKS reads in an
#    iteration becomes its own task/thread, run concurrently.
# ---------------------------------------------------------------------

async def asyncio_scalar_read_concurrent():
    print('Running asyncio threadpool scalar pread, concurrent per iteration')
    fd = os.open(SRC, os.O_RDONLY)

    start = time.perf_counter()
    for base in offsets():
        tasks = [
            asyncio.to_thread(os.pread, fd, CHUNK_SIZE, base + i * CHUNK_SIZE)
            for i in range(NR_CHUNKS)
        ]
        await asyncio.gather(*tasks)
    elapsed = time.perf_counter() - start

    os.close(fd)
    return elapsed


async def uvloop_readv():
    print('Running uvloop threadpool os.readv')
    return await asyncio_readv()


async def uvloop_scalar_read_concurrent():
    print('Running uvloop threadpool scalar pread, concurrent per iteration')
    return await asyncio_scalar_read_concurrent()


# ---------------------------------------------------------------------
# 5. puring readv: one IORING_OP_READV submission per iteration,
#    scattering into NR_CHUNKS buffers - no worker threads at all.
# ---------------------------------------------------------------------

async def puring_readv():
    print('Running puring readv (single SQE per iteration)')
    f = await puring.open_file(path=SRC)
    buffers = [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]

    start = time.perf_counter()
    for off in offsets():
        await f.readv(buffers=buffers, offset=off)
    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


# ---------------------------------------------------------------------
# 6. puring scalar: NR_CHUNKS separate read() SQEs per iteration,
#    submitted concurrently and awaited together.
# ---------------------------------------------------------------------

async def puring_scalar_read_concurrent():
    print('Running puring scalar read, concurrent per iteration')
    f = await puring.open_file(path=SRC)

    start = time.perf_counter()
    for base in offsets():
        tasks = [
            f.read(offset=base + i * CHUNK_SIZE, size=CHUNK_SIZE)
            for i in range(NR_CHUNKS)
        ]
        await asyncio.gather(*tasks)
    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


# ---------------------------------------------------------------------
# 7. puring readv with FIXED (pre-registered) buffers - skips the
#    per-call buffer mapping/pinning that the default mode pays for.
# ---------------------------------------------------------------------

async def puring_readv_fixed():
    print('Running puring readv FIXED buffers')
    f = await puring.open_file(path=SRC)
    buffers = [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]
    loop = asyncio.get_running_loop()

    start = time.perf_counter()
    with loop.buffer_mode(
        mode=puring.BUFFER_MODE.FIXED,
        payload_type=puring.PAYLOAD_TYPE.IOVEC,
        buffers=buffers,
    ):
        for off in offsets():
            await f.readv(buffers=buffers, offset=off)
    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


def run():
    print(f'{CHUNK_SIZE=}, {NR_CHUNKS=}, {ITERATIONS=}, total={TOTAL_SIZE / (1024 ** 3):.2f} GiB')

    results = []

    t = sync_readv()
    results.append(('sync readv', t))

    t = sync_scalar_read()
    results.append(('sync scalar pread', t))

    t = asyncio.run(asyncio_readv())
    results.append(('asyncio readv', t))

    t = asyncio.run(asyncio_scalar_read_concurrent())
    results.append(('asyncio scalar concurrent', t))

    if is_uvloop_installed:
        uvloop.install()

        t = asyncio.run(uvloop_readv())
        results.append(('uvloop readv', t))

        t = asyncio.run(uvloop_scalar_read_concurrent())
        results.append(('uvloop scalar concurrent', t))

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        t = runner.run(puring_readv())
        results.append(('puring readv', t))

        t = runner.run(puring_scalar_read_concurrent())
        results.append(('puring scalar concurrent', t))

        t = runner.run(puring_readv_fixed())
        results.append(('puring readv FIXED', t))

    print('\n==== RESULTS ====')
    for name, sec in results:
        print(f'{name:28s} {mbps(sec):10.2f} MB/s   ({sec:.3f}s)')


if __name__ == '__main__':
    os.makedirs(FILES_FOLDER, exist_ok=True)

    try:
        make_source()
        run()
    finally:
        shutil.rmtree(FILES_FOLDER)
