import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test
from tests.python_tests.tests_utils.socket_pair import make_connected_pair


@pytest_parametrize(
    ('buffers'),
    (
        pytest_param(buffers=123, id='buffers_wrong_type_int'),
        pytest_param(buffers=None, id='buffers_wrong_type_none'),
    ),
)
@puring_test
async def test_sendmsg__validation_error(buffers):
    server_conn, client_sock, server_sock = await make_connected_pair()

    with pytest.raises(expected_exception=TypeError):
        client_sock.sendmsg(buffers=buffers)

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_sendmsg__closed_socket_raises_error():
    sock = await puring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.sendmsg(buffers=[b'hello'])


@puring_test
async def test_sendmsg__success():
    server_conn, client_sock, server_sock = await make_connected_pair()

    chunks = [b'hello ', b'world', b'!']
    total_len = sum(len(c) for c in chunks)

    written = await client_sock.sendmsg(buffers=chunks)
    assert written == total_len

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
