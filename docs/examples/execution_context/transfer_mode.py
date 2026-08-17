import sys
sys.path.insert(0, '')
import asyncio
import aio_uring
from _common import simple_socket_example

async def transfer_mode():
    loop = asyncio.get_running_loop()
    with loop.transfer_mode(mode=aio_uring.TRANSFER_MODE.NORMAL):
        await simple_socket_example()


if __name__ == '__main__':
    asyncio.run(transfer_mode(), loop_factory=aio_uring.AioUringLoop)
