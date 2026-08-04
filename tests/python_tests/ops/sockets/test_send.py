import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test
from tests.python_tests.tests_utils.socket_pair import make_connected_pair


@pytest_parametrize(
    ('data'),
    (
        pytest_param(data=123, id='data_wrong_type_int'),
        pytest_param(data=None, id='data_wrong_type_none'),
    ),
)
@puring_test
async def test_send__validation_error(data):
    server_conn, client_sock, server_sock = await make_connected_pair()

    with pytest.raises(expected_exception=TypeError):
        client_sock.send(data=data)

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_send__no_req_params():
    server_conn, client_sock, server_sock = await make_connected_pair()

    with pytest.raises(expected_exception=TypeError):
        client_sock.send()

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_send__closed_socket_raises_error():
    sock = await puring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.send(data=b'hello')


@puring_test
async def test_send__success():
    server_conn, client_sock, server_sock = await make_connected_pair()

    data = b'hello world'
    written = await client_sock.send(data=data)
    assert written == len(data)

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_send__bytearray_input():
    server_conn, client_sock, server_sock = await make_connected_pair()

    data = bytearray(b'from bytearray')
    written = await client_sock.send(data=data)
    assert written == len(data)

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
