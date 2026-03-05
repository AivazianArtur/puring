import asyncio
from config import *

async def handle(r, w):
    try:
        while True:
            data = await r.readexactly(MSG_SIZE)
            w.write(data)
            await w.drain()
    except:
        pass
    finally:
        w.close()

async def main():
    srv = await asyncio.start_server(
        handle,
        HOST,
        PORT,
        backlog=65535,
    )

    async with srv:
        await srv.serve_forever()

asyncio.run(main())
