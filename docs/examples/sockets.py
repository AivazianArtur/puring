import sys

sys.path.insert(0, '')

import asyncio
import aio_uring

HOST = '127.0.0.1'
PORT = 12878

async def main():
    server_sock = await aio_uring.prep_socket()
    print(f'{server_sock = }')
    await server_sock.bind(HOST, PORT)
    await server_sock.listen(1)
    print(f'Server listening on {HOST}:{PORT}')

    client_sock = await aio_uring.prep_socket()

    await client_sock.connect(HOST, PORT)
    print('Client connected')

    server_conn = await server_sock.accept()
    print(f'{server_conn = }')
    message = b'Hello from client!'
    await client_sock.send(message)
    print(f'Client sent message')

    received_data = await server_conn.recv()

    result = received_data.decode()
    print(f'Server received: {result}')
    assert result == message.decode()

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
    print('Sockets closed')


# asyncio.run(main(), loop_factory=aio_uring.AioUringLoop)
with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
    runner.run(main())
