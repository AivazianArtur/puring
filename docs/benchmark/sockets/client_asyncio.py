import asyncio, time
from config import *
from metrics import record, report

async def worker():

    r, w = await asyncio.open_connection(HOST, PORT)

    for _ in range(MESSAGES):
        t0 = time.perf_counter()

        w.write(PAYLOAD)
        await w.drain()

        await r.readexactly(MSG_SIZE)

        record(t0)

    w.close()
    await w.wait_closed()


async def main(name='asyncio'):

    await asyncio.sleep(WARMUP)

    start = time.perf_counter()

    await asyncio.gather(
        *(worker() for _ in range(CONNECTIONS))
    )

    elapsed = time.perf_counter() - start

    report(f"{name} selector",
           CONNECTIONS * MESSAGES,
           elapsed)

asyncio.run(main())
