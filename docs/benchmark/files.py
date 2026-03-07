import asyncio
import os
import shutil
import time
import sys

sys.path.insert(0, '')

# WARNING: Both linux only
import puring
is_uvloop_installed = False
try:
    import uvloop
    is_uvloop_installed = True
except ImportError:
    pass

CHUNK_SIZE = 8 * 1024
ITERATIONS = 2000
DATA = b'x' * CHUNK_SIZE

FILES_FOLDER = 'docs/assets/benchmark_temp/'

FILE_ASYNC = FILES_FOLDER + 'async_test.bin'
FILE_STD = FILES_FOLDER + 'std_test.bin'
FILE_PURING_SEQ = FILES_FOLDER + 'puring_seq.bin'
FILE_PURING_SEQ__INIT = FILES_FOLDER + 'puring_seq__init.bin'
FILE_UV = FILES_FOLDER + 'uv_test.bin'


def mbps(seconds: float):
    total = CHUNK_SIZE * ITERATIONS
    return total / seconds / (1024 ** 2)


async def standard_write():
    print('Running synchronous write')

    start = time.perf_counter()

    with open(FILE_STD, 'wb') as f:
        for _ in range(ITERATIONS):
            f.write(DATA)
        f.flush()
        os.fsync(f.fileno())

    return time.perf_counter() - start


async def asyncio_thread_write():
    print('Running asyncio threadpool write')

    def worker():
        with open(FILE_ASYNC, 'wb') as f:
            for _ in range(ITERATIONS):
                f.write(DATA)
            f.flush()
            os.fsync(f.fileno())

    start = time.perf_counter()
    await asyncio.to_thread(worker)
    return time.perf_counter() - start


async def uvloop_write():
    print('Running uvloop + threadpool write')

    def worker():
        with open(FILE_UV, 'wb') as f:
            for _ in range(ITERATIONS):
                f.write(DATA)
            f.flush()
            os.fsync(f.fileno())

    start = time.perf_counter()
    await asyncio.to_thread(worker)
    return time.perf_counter() - start


async def puring_write_sequential():
    print('Running io_uring sequential write')

    loop = puring.uring()
    loop.add_reader()

    fd = await loop.open(path=FILE_PURING_SEQ)

    start = time.perf_counter()

    for _ in range(ITERATIONS):
        await loop.write(fd, DATA)

    await loop.fsync(fd)
    await loop.close(fd)

    return time.perf_counter() - start


async def puring_write_sequential__include_init():
    print('Running io_uring sequential write, including init')

    start = time.perf_counter()

    loop = puring.uring()
    loop.add_reader()

    fd = await loop.open(path=FILE_PURING_SEQ__INIT)

    for _ in range(ITERATIONS):
        await loop.write(fd, DATA)

    # await loop.fsync(fd) // OPTIONAL
    await loop.close(fd)

    return time.perf_counter() - start


async def run():
    print(f'{CHUNK_SIZE=}, {ITERATIONS=}')

    results = []

    t = await standard_write()
    results.append(('standard', t))

    t = await asyncio_thread_write()
    results.append(('asyncio_thread', t))

    if is_uvloop_installed:
        uvloop.install()
        t = await uvloop_write()
        results.append(('uvloop_thread', t))

    t = await puring_write_sequential()
    results.append(('uring_seq', t))

    # t = await puring_write_sequential__include_init()
    # results.append(('uring_seq__with_init', t))

    print('\n==== RESULTS ====')
    for name, sec in results:
        print(f'{name:16s} {mbps(sec):10.2f} MB/s   ({sec:.2f}s)')


if __name__ == '__main__':
    os.mkdir(FILES_FOLDER)
    asyncio.run(run())
    shutil.rmtree(FILES_FOLDER)
