import asyncio
import os
import shutil
import sys
import time
from concurrent.futures import ThreadPoolExecutor

is_uvloop_installed = False
try:
    import uvloop
    is_uvloop_installed = True
except ImportError:
    pass

sys.path.insert(0, '')

import aio_uring


CHUNK_SIZE = 256 * 1024
NR_CHUNKS = 32
ITERATIONS = 200

FILES_FOLDER = 'docs/benchmark_temp/'

FILE_STD = FILES_FOLDER + 'std.bin'
FILE_ASYNC = FILES_FOLDER + 'async.bin'
FILE_ASYNC_CONCURRENT = FILES_FOLDER + 'async_concurrent.bin'

FILE_UV = FILES_FOLDER + 'uv.bin'
FILE_UV_CONCURRENT = FILES_FOLDER + 'uv_concurrent.bin'

FILE_AIO_URING_VECTORED = FILES_FOLDER + 'vectored.bin'
FILE_AIO_URING_SEQ = FILES_FOLDER + 'scalar_seq.bin'
FILE_SCALAR_CONCURRENT = FILES_FOLDER + 'aio_uring_concurrent.bin'
FILE_CONCURRENT_FIXED = FILES_FOLDER + 'aio_uring_concurrent_fixed.bin'


DATA = [bytes([i % 256]) * CHUNK_SIZE for i in range(NR_CHUNKS)]


def mbps(total_bytes, seconds):
    return total_bytes / seconds / (1024 ** 2)


def open_write(path):
    return os.open(
        path,
        os.O_CREAT | os.O_WRONLY | os.O_TRUNC,
        0o644,
    )


def sync_writev():
    print('Running os.writev')

    fd = open_write(FILE_STD)

    start = time.perf_counter()

    for _ in range(ITERATIONS):
        os.writev(fd, DATA)

    os.fsync(fd)
    os.close(fd)

    return time.perf_counter() - start


async def asyncio_writev():
    print('Running asyncio threadpool os.writev')

    def worker():
        fd = open_write(FILE_ASYNC)

        for _ in range(ITERATIONS):
            os.writev(fd, DATA)

        os.fsync(fd)
        os.close(fd)

    start = time.perf_counter()

    await asyncio.to_thread(worker)

    return time.perf_counter() - start


async def asyncio_write_concurrent():
    print('Running asyncio threadpool write concurrent')

    async def run_once(fd, base_offset):
        tasks = [
            asyncio.to_thread(os.pwrite, fd, chunk, base_offset + i * CHUNK_SIZE)
            for i, chunk in enumerate(DATA)
        ]
        await asyncio.gather(*tasks)

    fd = open_write(FILE_ASYNC_CONCURRENT)
    stride = CHUNK_SIZE * NR_CHUNKS

    start = time.perf_counter()
    for it in range(ITERATIONS):
        await run_once(fd, it * stride)

    os.fsync(fd)
    os.close(fd)
    return time.perf_counter() - start


async def uvloop_writev():
    print('Running uvloop threadpool os.writev')

    def worker():
        fd = open_write(FILE_UV)

        for _ in range(ITERATIONS):
            os.writev(fd, DATA)

        os.fsync(fd)
        os.close(fd)

    start = time.perf_counter()

    await asyncio.to_thread(worker)
    return time.perf_counter() - start


async def uvloop_write_concurrent():
    print('Running uvloop threadpool write concurrent')
    fd = open_write(FILE_UV_CONCURRENT)

    async def run_once():
        tasks = [
            asyncio.to_thread(os.write, fd, chunk)
            for chunk in DATA
        ]

        await asyncio.gather(*tasks)

    start = time.perf_counter()

    for _ in range(ITERATIONS):
        await run_once()

    os.fsync(fd)
    os.close(fd)

    return time.perf_counter() - start


async def aio_uring_writev():
    print('Running aio_uring writev')
    f = await aio_uring.open_file(path=FILE_AIO_URING_VECTORED)

    start = time.perf_counter()

    for _ in range(ITERATIONS):
        await f.writev(buffers=DATA)

    await f.fsync()

    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


async def aio_uring_write():
    print('Running aio_uring write sequential')
    f = await aio_uring.open_file(path=FILE_AIO_URING_SEQ)

    start = time.perf_counter()

    for _ in range(ITERATIONS):
        for chunk in DATA:
            await f.write(chunk)

    await f.fsync()

    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


async def aio_uring_write_concurrent():
    print('Running aio_uring write concurrent')

    f = await aio_uring.open_file(path=FILE_SCALAR_CONCURRENT)

    start = time.perf_counter()

    for _ in range(ITERATIONS):
        tasks = [
            f.write(chunk)
            for chunk in DATA
        ]

        await asyncio.gather(*tasks)

    await f.fsync()

    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


async def aio_uring_write_concurrent__fixed():
    print('Running aio_uring write concurrent FIXED')
    f = await aio_uring.open_file(path=FILE_CONCURRENT_FIXED)

    loop = asyncio.get_running_loop()
    buf = bytearray(len(DATA))

    start = time.perf_counter()
    with loop.buffer_mode(
        mode=aio_uring.BUFFER_MODE.FIXED,
        payload_type=aio_uring.PAYLOAD_TYPE.IOVEC,
        buffers=[buf],
    ):
        for _ in range(ITERATIONS):
            tasks = [
                f.write(chunk)
                for chunk in DATA
            ]

            await asyncio.gather(*tasks)

    await f.fsync()

    elapsed = time.perf_counter() - start

    await f.close()
    return elapsed


def run():
    total_bytes = CHUNK_SIZE * NR_CHUNKS * ITERATIONS

    print(
        f'{CHUNK_SIZE=}, '
        f'{NR_CHUNKS=}, '
        f'{ITERATIONS=}, '
        f'total={total_bytes / 1024 / 1024:.1f} MB'
    )

    executor = ThreadPoolExecutor(max_workers=32)
    loop = asyncio.new_event_loop()
    loop.set_default_executor(executor)

    results = []

    t = sync_writev()
    results.append(('os.writev', t))

    t = asyncio.run(asyncio_writev())
    results.append(('asyncio writev', t))

    t = asyncio.run(asyncio_write_concurrent())
    results.append(('asyncio write concurrent', t))

    if is_uvloop_installed:
        uvloop.install()

        t = asyncio.run(uvloop_writev())
        results.append(('uvloop writev', t))

        t = asyncio.run(uvloop_write_concurrent())
        results.append(('uvloop write concurrent', t))


    with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:

        t = runner.run(aio_uring_writev())
        results.append(('aio_uring writev', t))

        t = runner.run(aio_uring_write())
        results.append(('aio_uring write seq', t))

        t = runner.run(aio_uring_write_concurrent())
        results.append(('aio_uring write concurrent', t))
    
        t = runner.run(aio_uring_write_concurrent__fixed())
        results.append(('aio_uring write concurrent FIXED', t))

    print('\n==== RESULTS ====')

    for name, sec in results:
        print(
            f'{name:28s} '
            f'{mbps(total_bytes, sec):10.2f} MB/s '
            f'({sec:.3f}s)'
        )


if __name__ == '__main__':
    os.makedirs(FILES_FOLDER, exist_ok=True)

    try:
        run()
    finally:
        shutil.rmtree(FILES_FOLDER)
