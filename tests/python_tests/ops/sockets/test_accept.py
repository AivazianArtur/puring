import sys

sys.path.insert(0, '')

import uringio
import pytest

from tests.python_tests.tests_utils.runner import uringio_test


@uringio_test
async def test_accept__closed_socket_raises_error():
    sock = await uringio.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.accept()


@uringio_test
async def test_accept__without_listen_raises_error():
    sock = await uringio.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    with pytest.raises(expected_exception=OSError):
        await sock.accept()

    await sock.close()


@uringio_test
async def test_accept__success():
    server_sock = await uringio.prep_socket()

    import socket as stdlib_socket

    tmp = stdlib_socket.socket(stdlib_socket.AF_INET, stdlib_socket.SOCK_STREAM)
    tmp.bind(('127.0.0.1', 0))
    _, port = tmp.getsockname()
    tmp.close()

    await server_sock.bind(host='127.0.0.1', port=port)
    await server_sock.listen(backlog=1)

    accept_future = server_sock.accept()

    client_sock = await uringio.prep_socket()
    await client_sock.connect(host='127.0.0.1', port=port)

    server_conn = await accept_future
    assert server_conn is not None
    assert server_conn is not server_sock

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
