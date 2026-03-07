import asyncio
import uvloop

asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

from client_asyncio import main

asyncio.run(main(name='uvloop'))
