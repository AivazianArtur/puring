import sys
sys.path.insert(0, 'build/lib.linux-x86_64-cpython-312')
import asyncio
import puring

HOST = '127.0.0.1'
PORT = 12345

async def main():
    loop = puring.uring(registry_size=8)
    print('UringLoop created:', loop)

    puring.add_uring_reader(loop)
    print('Reader added')

    server_sock = await loop.tcp_socket()
    print(f'{server_sock = }')
    await server_sock.bind(HOST, PORT)
    await server_sock.listen(1)
    print(f'Server listening on {HOST}:{PORT}')

    accept_future = server_sock.accept()

    client_loop = puring.uring(registry_size=8)
    puring.add_uring_reader(client_loop)
    client_sock_future = client_loop.tcp_socket()
    client_sock = await client_sock_future
    await client_sock.connect(HOST, PORT)
    print('Client connected')

    server_conn, addr = await accept_future
    print(f'Server accepted connection from {addr}')

    message = b'Hello from client!'
    bytes_sent = await client_sock.send(message)
    print(f'Client sent {bytes_sent} bytes')

    received_data = await server_conn.recv()
    print(f'Server received: {received_data.decode()}')

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
    print('Sockets closed')

    loop.close_loop()
    client_loop.close_loop()
    print('Uring loops closed')


asyncio.run(main())
