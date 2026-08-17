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

import aio_uring


# ---- Config ---------------------------------------------------------------
# Each "iteration" reads NR_CHUNKS * CHUNK_SIZE contiguous bytes from the
# source file, either as one vectored readv() into NR_CHUNKS buffers, or as
# NR_CHUNKS separate scalar reads.
#
# We run every variant twice:
#   COLD  - page cache for the file is dropped (POSIX_FADV_DONTNEED) right
#           before each timed call, so the kernel/io_uring actually has to
#           go to the block device. This is what you want if the question
#           is "does a vectored/io_uring read save real disk I/O time".
#   HOT   - the file is pre-warmed into the page cache and never dropped,
#           so every read is a memcpy from RAM. This isolates pure
#           submission/syscall/dispatch overhead, with no device latency
#           to hide behind.
#
# Comparing HOT numbers across implementations tells you about CPU/syscall
# overhead. Comparing COLD numbers tells you about real I/O concurrency.
# Don't compare a HOT number to a COLD number - they measure different things.

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


def drop_cache(path=SRC):
    """Evict the file's pages from the kernel page cache.

    Path-based (not tied to whichever fd later reads the file) so it works
    the same way whether the next reader is os.readv, os.pread, or aio_uring.
    Best-effort: only reliable when nothing else in the system is pinning
    those pages, which holds for a single-process benchmark like this one.
    """
    fd = os.open(path, os.O_RDONLY)
    try:
        os.posix_fadvise(fd, 0, 0, os.POSIX_FADV_DONTNEED)
    finally:
        os.close(fd)


def warm_cache(path=SRC):
    """Read the whole file once sequentially so it's resident in the page
    cache before the HOT suite starts timing anything."""
    fd = os.open(path, os.O_RDONLY)
    buf = bytearray(4 * 1024 * 1024)
    try:
        while os.readv(fd, [buf]):
            pass
    finally:
        os.close(fd)


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
# 5. aio_uring readv: one IORING_OP_READV submission per iteration,
#    scattering into NR_CHUNKS buffers - no worker threads at all.
# ---------------------------------------------------------------------

async def aio_uring_readv():
    print('Running aio_uring readv (single SQE per iteration)')
    f = await aio_uring.open_file(path=SRC)
    buffers = [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]

    start = time.perf_counter()
    for off in offsets():
        await f.readv(buffers=buffers, offset=off)
    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


# ---------------------------------------------------------------------
# 6. aio_uring scalar: NR_CHUNKS separate read() SQEs per iteration,
#    submitted concurrently and awaited together.
# ---------------------------------------------------------------------

async def aio_uring_scalar_read_concurrent():
    print('Running aio_uring scalar read, concurrent per iteration')
    f = await aio_uring.open_file(path=SRC)

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
# 7. aio_uring readv with FIXED (pre-registered) buffers - skips the
#    per-call buffer mapping/pinning that the default mode pays for.
# ---------------------------------------------------------------------

async def aio_uring_readv_fixed():
    print('Running aio_uring readv FIXED buffers')
    f = await aio_uring.open_file(path=SRC)
    buffers = [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]
    loop = asyncio.get_running_loop()

    start = time.perf_counter()

    # If you'll face memory allocation error, up your limit ulimit -Hl
    with loop.buffer_mode(
        mode=aio_uring.BUFFER_MODE.FIXED,
        payload_type=aio_uring.PAYLOAD_TYPE.IOVEC,
        buffers=buffers,
    ):
        for off in offsets():
            await f.readv(buffers=buffers, offset=off)
    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


def run_suite(cold):
    """Run every read variant once. If `cold` is True, the page cache for
    SRC is dropped immediately before each timed call, so the read has to
    go to the device. If False, the caller is expected to have already
    warmed the cache (see run()) and nothing is dropped in between."""

    def timed_sync(name, func):
        if cold:
            drop_cache()
        t = func()
        results.append((name, t))

    def timed_async(name, coro_func, runner=None):
        if cold:
            drop_cache()
        if runner is None:
            t = asyncio.run(coro_func())
        else:
            t = runner.run(coro_func())
        results.append((name, t))

    results = []

    timed_sync('sync readv', sync_readv)
    timed_sync('sync scalar pread', sync_scalar_read)

    timed_async('asyncio readv', asyncio_readv)
    timed_async('asyncio scalar concurrent', asyncio_scalar_read_concurrent)

    if is_uvloop_installed:
        uvloop.install()
        timed_async('uvloop readv', uvloop_readv)
        timed_async('uvloop scalar concurrent', uvloop_scalar_read_concurrent)

    with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
        timed_async('aio_uring readv', aio_uring_readv, runner)
        timed_async('aio_uring scalar concurrent', aio_uring_scalar_read_concurrent, runner)
        # timed_async('aio_uring readv FIXED', aio_uring_readv_fixed, runner)

    return results


def print_results(title, results):
    print(f'\n==== {title} ====')
    for name, sec in results:
        print(f'{name:28s} {mbps(sec):10.2f} MB/s   ({sec:.3f}s)')


def run():
    print(f'{CHUNK_SIZE=}, {NR_CHUNKS=}, {ITERATIONS=}, total={TOTAL_SIZE / (1024 ** 3):.2f} GiB')

    # COLD: drop the cache before every single call -> real device I/O.
    cold_results = run_suite(cold=True)

    # HOT: warm the cache once, then never drop it -> pure dispatch overhead.
    warm_cache()
    hot_results = run_suite(cold=False)

    print_results('COLD (real device I/O, cache dropped per call)', cold_results)
    print_results('HOT (page cache, no device I/O)', hot_results)


if __name__ == '__main__':
    os.makedirs(FILES_FOLDER, exist_ok=True)

    try:
        make_source()
        run()
    finally:
        shutil.rmtree(FILES_FOLDER)
