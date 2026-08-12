import sys
sys.path.insert(0, '')
import asyncio
import puring

from _common import simple_file_example

async def buffer_mode():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)
    with loop.buffer_mode(mode=puring.BUFFER_MODE.FIXED, buffers=[buf], payload_type=puring.PAYLOAD_TYPE.IOVEC):
        await simple_file_example()


if __name__ == '__main__':
    asyncio.run(buffer_mode(), loop_factory=puring.PuringLoop)
