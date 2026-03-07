import sys

sys.path.insert(0, '')

import asyncio
import puring

HOST = '127.0.0.1'
PORT = 12345

async def main():
    loop = puring.uring(registry_size=8)
    print('UringLoop created:', loop)

    loop.add_reader()
    print('Reader added')

    server_sock = await loop.tcp_socket()
    print(f'{server_sock = }')
    await server_sock.bind(HOST, PORT)
    await server_sock.listen(1)
    print(f'Server listening on {HOST}:{PORT}')

    accept_future = server_sock.accept(1024, 0)

    client_loop = puring.uring(registry_size=8)
    client_loop.add_reader()
    client_sock = await client_loop.tcp_socket()
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

    loop.close_loop()
    client_loop.close_loop()
    print('Uring loops closed')


asyncio.run(main())
