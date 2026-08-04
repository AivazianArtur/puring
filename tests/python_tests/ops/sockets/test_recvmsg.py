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
    ),
)
@puring_test
async def test_recvmsg__validation_error(buffers):
    server_conn, client_sock, server_sock = await make_connected_pair()

    with pytest.raises(expected_exception=TypeError):
        server_conn.recvmsg(buffers=buffers)

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()


@puring_test
async def test_recvmsg__closed_socket_raises_error():
    sock = await puring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.recvmsg()


@puring_test
async def test_recvmsg__success():
    server_conn, client_sock, server_sock = await make_connected_pair()

    data = b'hello via recvmsg'
    buf = bytearray(len(data))

    recvmsg_future = server_conn.recvmsg(buffers=[buf])
    await client_sock.sendmsg(buffers=[data])

    await recvmsg_future
    assert bytes(buf) == data

    await client_sock.close()
    await server_conn.close()
    await server_sock.close()
