import sys
sys.path.insert(0, '')
import asyncio
import puring


TEMPFILE = 'docs/assets/tempfile.txt'


async def user_buffer():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    async with loop.buffer_mode(mode='PROVIDED'):
        uring_file = await puring.open_file(path=TEMPFILE)

        data = b'Hello, puring!\n'
        bytes_written = await uring_file.write(data=data, buf=buf)
        print('Bytes written:', bytes_written)


async def lib_buffer():
    loop = asyncio.get_running_loop()
    # async with loop.execution_context(
    #     buffer_mode='PROVIDED',
    #     stream_strategy='ZERO_COPY',
    # )
    async with loop.buffer_mode(mode='PROVIDED'):
        uring_file = await puring.open_file(path=TEMPFILE)

        data = b'Hello, puring!\n'
        bytes_written = await uring_file.write(data=data)
        print('Bytes written:', bytes_written)


if __name__ == '__main__':
    asyncio.run(user_buffer(), loop_factory=puring.PuringLoop)
    asyncio.run(lib_buffer(), loop_factory=puring.PuringLoop)
