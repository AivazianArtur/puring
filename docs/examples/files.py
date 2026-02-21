import sys
sys.path.insert(0, 'build/lib.linux-x86_64-cpython-312')
import asyncio
import puring


async def main():
    loop = puring.uring(registry_size=8)
    print('UringLoop created:', loop)

    puring.add_uring_reader(loop)
    print('Reader added')
    fd = await loop.open(path='testfile.txt')
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
