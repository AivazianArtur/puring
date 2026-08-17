import asyncio
import os
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

CHUNK_SIZE = 1024 * 1024
ITERATIONS = 2000
DATA = b'x' * CHUNK_SIZE

FILES_FOLDER = 'docs/assets/benchmark_temp/'

FILE_ASYNC = FILES_FOLDER + 'async_test.bin'
FILE_STD = FILES_FOLDER + 'std_test.bin'
FILE_AIO_URING_SEQ = FILES_FOLDER + 'aio_uring_seq.bin'
FILE_AIO_URING_SEQ_FIXED = FILES_FOLDER + 'aio_uring_seq_fixed.bin'
FILE_AIO_URING_SEQ__INIT = FILES_FOLDER + 'aio_uring_seq__init.bin'
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


async def aio_uring_write_sequential():
    print('Running io_uring sequential write')
    uring_file = await aio_uring.open_file(path=FILE_AIO_URING_SEQ)
    start = time.perf_counter()
    for _ in range(ITERATIONS):
        await uring_file.write(DATA)
    await uring_file.fsync()
    await uring_file.close()
    return time.perf_counter() - start


async def aio_uring_write_sequential__fixed():
    print('Running io_uring sequential write in FIXED mode')
    uring_file = await aio_uring.open_file(path=FILE_AIO_URING_SEQ_FIXED)
    buf = bytearray(DATA)
    loop = asyncio.get_running_loop()
    start = time.perf_counter()
    with loop.buffer_mode(
        mode=aio_uring.BUFFER_MODE.FIXED,
        payload_type=aio_uring.PAYLOAD_TYPE.IOVEC,
        buffers=[buf],
    ):
        for _ in range(ITERATIONS):
            await uring_file.write(DATA)
    await uring_file.fsync()
    await uring_file.close()
    return time.perf_counter() - start


def run():
    print(f'{CHUNK_SIZE=}, {ITERATIONS=}')

    results = []

    t = asyncio.run(standard_write())
    results.append(('standard', t))

    t = asyncio.run(asyncio_thread_write())
    results.append(('asyncio_thread', t))

    if is_uvloop_installed:
        uvloop.install()
        t = asyncio.run(uvloop_write())
        results.append(('uvloop_thread', t))

    with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
        t = runner.run(aio_uring_write_sequential())
        results.append(('aio_uring_seq', t))
    with asyncio.Runner(loop_factory=aio_uring.AIoUringLoop) as runner:
        t = runner.run(aio_uring_write_sequential__fixed())
        results.append(('aio_uring_seq FIXED', t))

    print('\n==== RESULTS ====')
    for name, sec in results:
        print(f'{name:16s} {mbps(sec):10.2f} MB/s   ({sec:.2f}s)')


if __name__ == '__main__':
    try:
        os.mkdir(FILES_FOLDER)
    except FileExistsError:
        pass

    try:
        run()
    finally:
        shutil.rmtree(FILES_FOLDER)
