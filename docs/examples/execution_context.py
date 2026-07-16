import sys
sys.path.insert(0, '')
import asyncio
import puring


TEMPFILE = 'docs/assets/tempfile.txt'


async def buffer_mode():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)
    with loop.buffer_mode(mode=puring.BUFFER_MODE.FIXED, buffers=[buf]):
        uring_file = await puring.open_file(path=TEMPFILE)
        data = b'Hello, puring!\n'
        bytes_written = await uring_file.write(data=data)
        print('Bytes written:', bytes_written)


async def transfer_mode():
    loop = asyncio.get_running_loop()
    HOST = '127.0.0.1'
    PORT = 12878

    with loop.transfer_mode(mode=puring.TRANSFER_MODE.ZERO_COPY):
        server_sock = await puring.prep_socket()
        print(f'{server_sock = }')
        await server_sock.bind(HOST, PORT)
        await server_sock.listen(1)
        print(f'Server listening on {HOST}:{PORT}')

        accept_future = server_sock.accept()
        client_sock = await puring.prep_socket()

        await client_sock.connect(HOST, PORT)
        print('Client connected')

        server_conn = await accept_future
        print(f'{server_conn = }')
        message = b'Hello from client!'
        await client_sock.send(message)
        print(f'Client sent message')

        received_data_r = server_conn.recv()
        received_data = await received_data_r
        print(f'Server received: {received_data.decode()}')

        await client_sock.close()
        await server_conn.close()
        await server_sock.close()
        print('Sockets closed')


async def lib_buffer():
    loop = asyncio.get_running_loop()
    # async with loop.execution_context(
    #     buffer_mode='PROVIDED',
    #     stream_strategy='ZERO_COPY',
    # )
    with loop.buffer_mode(mode=puring.BUFFER_MODE.PROVIDED):
        uring_file = await puring.open_file(path=TEMPFILE)

        data = b'Hello, puring!\n'
        bytes_written = await uring_file.write(data=data)
        print('Bytes written:', bytes_written)


if __name__ == '__main__':
    asyncio.run(buffer_mode(), loop_factory=puring.PuringLoop)
    asyncio.run(transfer_mode(), loop_factory=puring.PuringLoop)
    # asyncio.run(lib_buffer(), loop_factory=puring.PuringLoop)
