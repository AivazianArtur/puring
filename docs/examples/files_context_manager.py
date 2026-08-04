import os
import sys

sys.path.insert(0, '')

import asyncio
import puring


TEMPFILE = 'docs/assets/tempfile.txt'


async def main():
    async with await puring.open_file(path=TEMPFILE) as uring_file:
        print('File opened, fd:', uring_file)

        data = b'Hello, puring!\n'
        bytes_written = await uring_file.write(data=data)
        print('Bytes written:', bytes_written)

        read_data = await uring_file.read()

        result = read_data.decode()
        print('Read data:', result)
        assert result == data.decode()

    print('File closed')


asyncio.run(main(), loop_factory=puring.PuringLoop)
os.remove(TEMPFILE)
