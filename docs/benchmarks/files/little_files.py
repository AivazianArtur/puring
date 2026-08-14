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


# ---- Config -----------------------------------------------------------
# This isn't a throughput test - the files are tiny and stay warm in the
# dentry/inode cache after the first pass. It's isolating the fixed cost
# of open()+close() itself: one syscall pair per file for the sync/thread
# variants, vs one (or two, batched) io_uring submissions for puring.
# That fixed cost is what dominates workloads like serving many small
# static assets or scanning a directory tree.

NR_FILES = 5000
FILE_SIZE = 4096  # 4 KiB, just enough to be a "real" file, not empty

DEPTHS = [1, 8, 32, 128, 512]

FILES_FOLDER = 'docs/benchmark_open_close/'
FILES = [
    os.path.join(FILES_FOLDER, f'file_{i}.bin')
    for i in range(NR_FILES)
]


def ops_per_sec(seconds):
    return NR_FILES / seconds


def avg_latency_us(seconds):
    return seconds / NR_FILES * 1_000_000


def prepare():
    os.makedirs(FILES_FOLDER, exist_ok=True)
    block = os.urandom(FILE_SIZE)
    for path in FILES:
        with open(path, 'wb') as f:
            f.write(block)


# -------------------------------------------------------------------
# 1. sync baseline: one os.open() + os.close() pair per file, serial.
# -------------------------------------------------------------------

def sync_open_close():
    print('Running sync os.open/os.close (serial)')
    start = time.perf_counter()
    for path in FILES:
        fd = os.open(path, os.O_RDONLY)
        os.close(fd)
    return time.perf_counter() - start


# -------------------------------------------------------------------
# 2. asyncio threadpool, swept over concurrency depth: each file's
#    open+close pair runs on a worker thread, bounded by a semaphore.
# -------------------------------------------------------------------

async def asyncio_open_close(depth):
    print(f'Running asyncio threadpool open/close, depth={depth}')
    sem = asyncio.Semaphore(depth)

    async def one(path):
        async with sem:
            fd = await asyncio.to_thread(os.open, path, os.O_RDONLY)
            await asyncio.to_thread(os.close, fd)

    start = time.perf_counter()
    await asyncio.gather(*[one(p) for p in FILES])
    return time.perf_counter() - start


async def uvloop_open_close(depth):
    print(f'Running uvloop threadpool open/close, depth={depth}')
    return await asyncio_open_close(depth)


# -------------------------------------------------------------------
# 3. puring: open+close as two io_uring SQEs per file, no worker
#    threads involved. Swept over concurrency depth the same way, to
#    show how submission batching changes the picture as depth grows.
# -------------------------------------------------------------------

async def puring_open_close(depth):
    print(f'Running puring open/close (io_uring), depth={depth}')
    sem = asyncio.Semaphore(depth)

    async def one(path):
        async with sem:
            f = await puring.open_file(path=path)
            await f.close()

    start = time.perf_counter()
    await asyncio.gather(*[one(p) for p in FILES])
    return time.perf_counter() - start


def run():
    print(f'{NR_FILES=}, {FILE_SIZE=}')
    prepare()

    results = []

    t = sync_open_close()
    results.append(('sync', 1, t))

    for depth in DEPTHS:
        t = asyncio.run(asyncio_open_close(depth))
        results.append(('asyncio', depth, t))

    if is_uvloop_installed:
        uvloop.install()
        for depth in DEPTHS:
            t = asyncio.run(uvloop_open_close(depth))
            results.append(('uvloop', depth, t))

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        for depth in DEPTHS:
            t = runner.run(puring_open_close(depth))
            results.append(('puring', depth, t))

    print('\n==== RESULTS ====')
    print(f'{"backend":10s} {"depth":>6s} {"ops/s":>10s} {"avg latency":>14s} {"time":>8s}')
    for name, depth, sec in results:
        print(
            f'{name:10s} {depth:6d} '
            f'{ops_per_sec(sec):10.0f} '
            f'{avg_latency_us(sec):11.1f} us '
            f'{sec:7.3f}s'
        )


if __name__ == '__main__':
    try:
        run()
    finally:
        shutil.rmtree(FILES_FOLDER, ignore_errors=True)
