import sys

sys.path.insert(0, '')

import socket as stdlib_socket

import aio_uring


async def make_connected_pair():
    """
    Returns (server_conn, client_sock, server_sock) — all three aio_uring sockets,
    server_conn <-> client_sock are a connected TCP pair.
    Caller is responsible for closing all three.
    """
    server_sock = await aio_uring.prep_socket()

    tmp = stdlib_socket.socket(stdlib_socket.AF_INET, stdlib_socket.SOCK_STREAM)
    tmp.bind(('127.0.0.1', 0))
    _, port = tmp.getsockname()
    tmp.close()

    await server_sock.bind(host='127.0.0.1', port=port)
    await server_sock.listen(backlog=1)

    accept_future = server_sock.accept()

    client_sock = await aio_uring.prep_socket()
    await client_sock.connect(host='127.0.0.1', port=port)

    server_conn = await accept_future

    return server_conn, client_sock, server_sock
