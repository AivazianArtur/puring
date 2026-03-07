import os
import sys

# sys.path.insert(0, 'build/lib.linux-x86_64-cpython-312')
sys.path.insert(0, '')

import asyncio
import puring


TEMPFILE = 'docs/assets/tempfile.txt'


async def main():
    loop = puring.uring(registry_size=8)
    print('UringLoop created:', loop)

    loop.add_reader()
    print('Reader added')
    fd = await loop.open(path=TEMPFILE)
    print('File opened, fd:', fd)

    data = b'Hello, puring!\n'
    bytes_written = await loop.write(fd, data=data)
    print('Bytes written:', bytes_written)

    read_data = await loop.read(fd=fd)
    print('Read data:', read_data.decode())

    await loop.close(fd=fd)
    print('File closed')

    loop.close_loop()
    print('Uring loop closed')

asyncio.run(main())
os.remove(TEMPFILE)
