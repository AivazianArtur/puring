import sys
sys.path.insert(0, 'build/lib.linux-x86_64-cpython-312')
import asyncio
import puring


async def main():
    loop = puring.uring(registry_size=8)
    print('UringLoop created:', loop)

    puring.add_uring_reader(loop)
    print('Reader added')
    future_open = loop.open(path='testfile.txt')
    print('File opened')
    fd = await future_open
    print('File opened awaited, fd:', fd)

    data = b'Hello io_uring!\n'
    future_write = loop.write(fd, data=data)
    bytes_written = await future_write
    print('Bytes written:', bytes_written)

    future_read = loop.read(fd=fd)
    read_data = await future_read
    print('Read data:', read_data.decode())

    future_close = loop.close(fd=fd)
    await future_close
    print('File closed')

    loop.close_loop()
    print('Uring loop closed')

asyncio.run(main())
