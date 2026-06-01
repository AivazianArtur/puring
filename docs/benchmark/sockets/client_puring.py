# Warning, not working now

import sys
import asyncio
import time

sys.path.insert(0, '')

import puring
from config import *
from metrics import record, report

connections = []

async def worker():
    sock = await puring.prep_socket()
    connections.append(sock)
    await sock.connect(HOST, PORT)
    for _ in range(MESSAGES):
        t0 = time.perf_counter()
        await sock.send(PAYLOAD)
        await sock.recv()
        record(t0)
    await sock.close()


async def run_benchmark():
    await asyncio.sleep(WARMUP)

    start = time.perf_counter()
    await asyncio.gather(*(worker() for _ in range(CONNECTIONS)))
    elapsed = time.perf_counter() - start

    report("puring io_uring", CONNECTIONS * MESSAGES, elapsed)

    await asyncio.sleep(0.5)


asyncio.run(run_benchmark(), loop_factory=puring.PuringLoop)
