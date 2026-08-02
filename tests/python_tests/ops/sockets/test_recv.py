import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test
from tests.python_tests.tests_utils.socket_pair import make_connected_pair


@pytest_parametrize(
    ('bufsize'),
    (
        pytest_param(bufsize='not_an_int', id='bufsize_wrong_type_str'),
        pytest_param(bufsize=1.5, id='bufsize_wrong_type_float'),
    ),
)
@puring_test
async def test_recv__validation_error(bufsize):
    server_conn, client_sock, server_sock = await make_connected_pair()

    with pytest.raises(expected_exception=TypeError):
        server_conn.recv(bufsize=bufsize)

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_recv__closed_socket_raises_error():
    sock = await puring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.recv()


@puring_test
async def test_recv__success():
    server_conn, client_sock, server_sock = await make_connected_pair()

    data = b'hello from client'
    recv_future = server_conn.recv(bufsize=1024)
    await client_sock.send(data=data)

    received = await recv_future
    assert received == data

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_recv__default_bufsize_success():
    server_conn, client_sock, server_sock = await make_connected_pair()

    data = b'small message'
    recv_future = server_conn.recv()
    await client_sock.send(data=data)

    received = await recv_future
    assert received == data

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
