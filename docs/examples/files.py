import os
import sys

sys.path.insert(0, '')

import asyncio
import puring


TEMPFILE = 'docs/assets/tempfile.txt'


async def main():
    loop = puring.PuringLoop(registry_size=8)
    print('PuringLoop created:', loop)

    uring_file = await loop.open(path=TEMPFILE)
    print('File opened, fd:', uring_file)

    data = b'Hello, puring!\n'
    bytes_written = await uring_file.write(data=data)
    print('Bytes written:', bytes_written)

    read_data = await uring_file.read()
    print('Read data:', read_data.decode())

    await uring_file.close()
    print('File closed')

    loop.close_loop()
    print('Puring loop closed')

asyncio.run(main())
os.remove(TEMPFILE)
