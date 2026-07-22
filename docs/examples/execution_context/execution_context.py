import sys
sys.path.insert(0, '')
import asyncio
import puring
from _common import simple_socket_example

TEMPFILE = 'docs/assets/tempfile.txt'


async def execution_context():
    loop = asyncio.get_running_loop()

    buf = bytearray(4096)
    with loop.execution_context(
        stream_strategy=puring.STREAM_STRATEGY.ONESHOT,
        buffer_mode=puring.BUFFER_MODE.FIXED,
        transfer_mode=puring.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        await simple_socket_example()

if __name__ == '__main__':
    asyncio.run(execution_context(), loop_factory=puring.PuringLoop)
