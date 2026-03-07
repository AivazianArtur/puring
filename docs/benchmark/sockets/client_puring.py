import sys
import asyncio
import time

sys.path.insert(0, '')

import puring
from config import *
from metrics import record, report

connections = []

async def worker(loop):
    sock = await loop.tcp_socket()
    connections.append(sock)
    await sock.connect(HOST, PORT)

    for _ in range(MESSAGES):
        t0 = time.perf_counter()
        await sock.send(PAYLOAD)
        await sock.recv()
        record(t0)

    await sock.close()

async def run_benchmark():
    loop = puring.uring(registry_size=8192)
    loop.add_reader()

    await asyncio.sleep(WARMUP)

    start = time.perf_counter()
    await asyncio.gather(*(worker(loop) for _ in range(CONNECTIONS)))
    elapsed = time.perf_counter() - start

    report("puring io_uring", CONNECTIONS * MESSAGES, elapsed)

    await asyncio.sleep(0.5)

    loop.close_loop()

asyncio.run(run_benchmark())
