import socket
import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest_parametrize(
    ('bufsize', 'domain'),
    (
        pytest_param(bufsize='not_an_int', domain=socket.AF_INET, id='bufsize_wrong_type_str'),
        pytest_param(bufsize=1.5, domain=socket.AF_INET, id='bufsize_wrong_type_float'),
        pytest_param(bufsize=1024, domain='not_an_int', id='domain_wrong_type_str'),
        pytest_param(bufsize=1024, domain=1.5, id='domain_wrong_type_float'),
    ),
)
@aio_uring_test
async def test_recvfrom__validation_error(bufsize, domain):
    sock = await aio_uring.prep_socket()

    with pytest.raises(expected_exception=TypeError):
        sock.recvfrom(bufsize=bufsize, host='127.0.0.1', port=0, domain=domain)

    await sock.close()


@aio_uring_test
async def test_recvfrom__closed_socket_raises_error():
    sock = await aio_uring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.recvfrom(host='127.0.0.1', port=0, domain=socket.AF_INET)


@aio_uring_test
async def test_recvfrom__success():
    sock = await aio_uring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)
    await sock.bind(host='127.0.0.1', port=0)

    tmp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    tmp.bind(('127.0.0.1', 0))
    _, port = tmp.getsockname()
    tmp.close()

    sock = await aio_uring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)
    await sock.bind(host='127.0.0.1', port=port)

    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data = b'hello via recvfrom'

    recv_future = sock.recvfrom(bufsize=1024, host='127.0.0.1', port=port, domain=socket.AF_INET)
    client.sendto(data, ('127.0.0.1', port))

    received = await recv_future
    assert received == data

    client.close()
    await sock.close()
