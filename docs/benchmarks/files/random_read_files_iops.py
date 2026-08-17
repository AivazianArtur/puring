import asyncio
import os
import random
import shutil
import time
import sys

sys.path.insert(0, '')

# WARNING: Both linux only
import aio_uring
is_uvloop_installed = False
try:
    import uvloop
    is_uvloop_installed = True
except ImportError:
    pass

# ---- Config -----------------------------------------------------------
FILES_FOLDER = 'docs/assets/benchmark_temp_iops/'

NUM_FILES = 300          # how many independent files we hammer at once
FILE_SIZE = 8 * 1024 * 1024   # 8 MB per file, enough room for random offsets
READ_SIZE = 4096          # 4 KiB random reads -> classic IOPS-bound workload
READS_PER_FILE = 200 

TOTAL_OPS = NUM_FILES * READS_PER_FILE

MAX_INFLIGHT = 4096

FILE_PATHS = [os.path.join(FILES_FOLDER, f'file_{i}.bin') for i in range(NUM_FILES)]


def _fmt(seconds: float):
    iops = TOTAL_OPS / seconds
    mbps = (TOTAL_OPS * READ_SIZE) / seconds / (1024 ** 2)
    return iops, mbps


def setup_files():
    os.mkdir(FILES_FOLDER)
    for path in FILE_PATHS:
        with open(path, 'wb') as f:
            f.write(os.urandom(FILE_SIZE))


def evict_page_cache():
    for path in FILE_PATHS:
        fd = os.open(path, os.O_RDONLY)
        try:
            os.posix_fadvise(fd, 0, 0, os.POSIX_FADV_DONTNEED)
        finally:
            os.close(fd)


def random_offsets():
    random.seed(42)
    max_offset = FILE_SIZE - READ_SIZE
    return [
        [random.randint(0, max_offset) for _ in range(READS_PER_FILE)]
        for _ in range(NUM_FILES)
    ]


async def standard_random_read(offsets):
    print('Running synchronous random pread (baseline, no concurrency)')

    start = time.perf_counter()
    for path, file_offsets in zip(FILE_PATHS, offsets):
        fd = os.open(path, os.O_RDONLY)
        try:
            for off in file_offsets:
                os.pread(fd, READ_SIZE, off)
        finally:
            os.close(fd)
    return time.perf_counter() - start


async def asyncio_thread_random_read(offsets):
    print('Running asyncio threadpool random pread (one task per read)')

    fds = [os.open(path, os.O_RDONLY) for path in FILE_PATHS]
    try:
        start = time.perf_counter()
        tasks = [
            asyncio.to_thread(os.pread, fd, READ_SIZE, off)
            for fd, file_offsets in zip(fds, offsets)
            for off in file_offsets
        ]
        await asyncio.gather(*tasks)
        return time.perf_counter() - start
    finally:
        for fd in fds:
            os.close(fd)


async def uvloop_random_read(offsets):
    print('Running uvloop threadpool random pread (one task per read)')

    fds = [os.open(path, os.O_RDONLY) for path in FILE_PATHS]
    try:
        start = time.perf_counter()
        tasks = [
            asyncio.to_thread(os.pread, fd, READ_SIZE, off)
            for fd, file_offsets in zip(fds, offsets)
            for off in file_offsets
        ]
        await asyncio.gather(*tasks)
        return time.perf_counter() - start
    finally:
        for fd in fds:
            os.close(fd)


# ---- aio_uring: every read is a plain io_uring SQE, no OS threads at all --
async def _bounded_read(sem, f, offset):
    # f.read() submits its SQE the moment it's called, so we must gate the
    # *call*, not just the await, or the semaphore does nothing.
    async with sem:
        return await f.read(offset=offset, size=READ_SIZE)


async def aio_uring_random_read(offsets):
    print('Running aio_uring concurrent random read (no threads)')

    files = await asyncio.gather(*(aio_uring.open_file(path=p) for p in FILE_PATHS))
    sem = asyncio.Semaphore(MAX_INFLIGHT)
    try:
        start = time.perf_counter()
        tasks = [
            _bounded_read(sem, f, off)
            for f, file_offsets in zip(files, offsets)
            for off in file_offsets
        ]
        await asyncio.gather(*tasks)
        return time.perf_counter() - start
    finally:
        await asyncio.gather(*(f.close() for f in files))


def run():
    print(f'{NUM_FILES=}, {READ_SIZE=}, {READS_PER_FILE=}, {TOTAL_OPS=}')
    setup_files()
    offsets = random_offsets()

    results = []

    evict_page_cache()
    t = asyncio.run(standard_random_read(offsets))
    results.append(('standard', t))

    evict_page_cache()
    t = asyncio.run(asyncio_thread_random_read(offsets))
    results.append(('asyncio_thread', t))

    if is_uvloop_installed:
        uvloop.install()
        evict_page_cache()
        t = asyncio.run(uvloop_random_read(offsets))
        results.append(('uvloop_thread', t))

    evict_page_cache()
    with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
        t = runner.run(aio_uring_random_read(offsets))
    results.append(('aio_uring_concurrent', t))

    print('\n==== RESULTS ====')
    print(f'{"backend":16s} {"IOPS":>12s} {"MB/s":>10s} {"time":>8s}')
    for name, sec in results:
        iops, mbps = _fmt(sec)
        print(f'{name:16s} {iops:12.0f} {mbps:10.2f} {sec:7.2f}s')


if __name__ == '__main__':
    try:
        run()
    finally:
        shutil.rmtree(FILES_FOLDER, ignore_errors=True)
