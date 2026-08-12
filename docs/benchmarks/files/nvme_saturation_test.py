import asyncio
import os
import shutil
import time
import sys

sys.path.insert(0, '')

import puring


FILES_FOLDER = "docs/nvme_saturation_temp"

NR_FILES = 8

FILE_SIZE = 1024 * 1024 * 1024  # 1 GiB each
CHUNK_SIZE = 1024 * 1024        # 1 MiB

DEPTHS = [1, 2, 4, 8, 16, 32, 64]


FILES = [
    os.path.join(FILES_FOLDER, f"file_{i}.bin")
    for i in range(NR_FILES)
]


DATA = b"x" * CHUNK_SIZE


def mbps(seconds):
    total = FILE_SIZE * NR_FILES
    return total / seconds / 1024 / 1024


def prepare():
    os.makedirs(FILES_FOLDER, exist_ok=True)


def open_write(path):
    fd = os.open(
        path,
        os.O_CREAT |
        os.O_WRONLY |
        os.O_TRUNC,
        0o644,
    )

    return fd



# --------------------------------------------------
# sync baseline
# --------------------------------------------------

def sync_write():
    print("sync write")
    start = time.perf_counter()
    fds = [
        open_write(p)
        for p in FILES
    ]

    try:
        for fd in fds:
            remaining = FILE_SIZE
            while remaining:
                os.write(fd, DATA)
                remaining -= CHUNK_SIZE

        for fd in fds:
            os.fsync(fd)

    finally:
        for fd in fds:
            os.close(fd)

    return time.perf_counter() - start


# --------------------------------------------------
# asyncio threadpool
# --------------------------------------------------

async def asyncio_write(depth):
    print(f"asyncio depth={depth}")

    sem = asyncio.Semaphore(depth)
    fds = [open_write(p) for p in FILES]

    async def write_one(fd, offset):
        async with sem:
            await asyncio.to_thread(os.pwrite, fd, DATA, offset)

    start = time.perf_counter()
    for fd in fds:
        chunks = FILE_SIZE // CHUNK_SIZE
        tasks = [write_one(fd, i * CHUNK_SIZE) for i in range(chunks)]
        await asyncio.gather(*tasks)

    for fd in fds:
        await asyncio.to_thread(os.fsync, fd)
    for fd in fds:
        os.close(fd)

    return time.perf_counter() - start

# --------------------------------------------------
# puring io_uring
# --------------------------------------------------

async def puring_write(depth):
    print(f"puring depth={depth}")

    files = await asyncio.gather(
        *[
            puring.open_file(path=p)
            for p in FILES
        ]
    )
    total_chunks = FILE_SIZE // CHUNK_SIZE

    start = time.perf_counter()
    for f in files:
        sem = asyncio.Semaphore(depth)
        async def submit(offset):

            async with sem:
                await f.write(
                    DATA,
                    offset=offset
                )

        tasks = [
            asyncio.create_task(
                submit(i * CHUNK_SIZE)
            )
            for i in range(total_chunks)
        ]
        await asyncio.gather(*tasks)


    await asyncio.gather(
        *[
            f.fsync()
            for f in files
        ]
    )

    elapsed = time.perf_counter() - start
    await asyncio.gather(
        *[
            f.close()
            for f in files
        ]
    )
    return elapsed



def run():
    prepare()
    total_gb = (
        FILE_SIZE *
        NR_FILES /
        1024 /
        1024 /
        1024
    )

    print(f"total write: {total_gb:.1f} GiB")
    t = sync_write()

    print(f"sync: {mbps(t):.2f} MB/s")

    for depth in DEPTHS:
        shutil.rmtree(FILES_FOLDER)
        prepare()
        t = asyncio.run(asyncio_write(depth))

        print(
            f"asyncio depth={depth}: "
            f"{mbps(t):.2f} MB/s"
        )

    # puring
    for depth in DEPTHS:
        shutil.rmtree(FILES_FOLDER)
        prepare()
        with asyncio.Runner(
            loop_factory=puring.PuringLoop
        ) as runner:

            t = runner.run(
                puring_write(depth)
            )
        print(
            f"puring depth={depth}: "
            f"{mbps(t):.2f} MB/s"
        )


if __name__ == "__main__":
    try:
        run()

    finally:
        shutil.rmtree(
            FILES_FOLDER,
            ignore_errors=True
        )
