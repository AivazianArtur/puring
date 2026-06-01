import os
import sys

sys.path.insert(0, '')

import asyncio
import puring


TEMPFILE = 'docs/assets/tempfile.txt'


async def main():
    uring_file = await puring.open_file(path=TEMPFILE)
    print('File opened, fd:', uring_file)

    data = b'Hello, puring!\n'
    bytes_written = await uring_file.write(data=data)
    print('Bytes written:', bytes_written)

    read_data = await uring_file.read()
    print('Read data:', read_data.decode())

    await uring_file.close()
    print('File closed')


asyncio.run(main(), loop_factory=puring.PuringLoop)
os.remove(TEMPFILE)
