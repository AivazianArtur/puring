import os
import sys

sys.path.insert(0, '')

import asyncio
import aio_uring


TEMPFILE = 'docs/assets/tempfile.txt'


async def main():
    uring_file = await aio_uring.open_file(path=TEMPFILE)
    print('File opened, fd:', uring_file)

    data = b'Hello, aio_uring!\n'
    bytes_written = await uring_file.write(data=data)
    print('Bytes written:', bytes_written)

    read_data = await uring_file.read()

    result = read_data.decode()
    print('Read data:', result)
    assert result == data.decode()

    await uring_file.close()
    print('File closed')


with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
    runner.run(main())

# asyncio.run(main(), loop_factory=aio_uring.AioUringLoop)
os.remove(TEMPFILE)
