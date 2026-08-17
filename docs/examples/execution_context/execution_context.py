import sys
sys.path.insert(0, '')
import asyncio
import aio_uring
from _common import simple_file_example

TEMPFILE = 'docs/assets/tempfile.txt'


async def execution_context():
    loop = asyncio.get_running_loop()

    buf = bytearray(4096)
    with loop.execution_context(
        stream_strategy=aio_uring.STREAM_STRATEGY.MULTISHOT,
        buffer_mode=aio_uring.BUFFER_MODE.FIXED,
        transfer_mode=aio_uring.TRANSFER_MODE.ZERO_COPY,
        buffers=[buf],
    ):
        await simple_file_example()

if __name__ == '__main__':
    asyncio.run(execution_context(), loop_factory=aio_uring.AioUringLoop)
