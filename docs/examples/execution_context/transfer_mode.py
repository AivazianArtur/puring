import asyncio
import uringio
from _common import simple_socket_example

async def transfer_mode():
    loop = asyncio.get_running_loop()
    with loop.transfer_mode(mode=uringio.TRANSFER_MODE.NORMAL):
        await simple_socket_example()


if __name__ == '__main__':
    asyncio.run(transfer_mode(), loop_factory=uringio.UringioLoop)
