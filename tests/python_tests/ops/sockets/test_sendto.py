import socket
import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest_parametrize(
    ('data', 'host', 'port', 'domain'),
    (
        pytest_param(data=123, host='127.0.0.1', port=0, domain=socket.AF_INET, id='data_wrong_type_int'),
        pytest_param(data=None, host='127.0.0.1', port=0, domain=socket.AF_INET, id='data_wrong_type_none'),
        pytest_param(data=b'x', host=None, port=0, domain=socket.AF_INET, id='host_wrong_type_none'),
        pytest_param(data=b'x', host=1234, port=0, domain=socket.AF_INET, id='host_wrong_type_int'),
        pytest_param(data=b'x', host='127.0.0.1', port=None, domain=socket.AF_INET, id='port_wrong_type_none'),
        pytest_param(data=b'x', host='127.0.0.1', port='not_an_int', domain=socket.AF_INET, id='port_wrong_type_str'),
        pytest_param(data=b'x', host='127.0.0.1', port=0, domain='not_an_int', id='domain_wrong_type_str'),
        pytest_param(data=b'x', host='127.0.0.1', port=0, domain=None, id='domain_wrong_type_none'),
        pytest_param(data=b'x', host='127.0.0.1', port=0, domain=1.5, id='domain_wrong_type_float'),
    ),
)
@aio_uring_test
async def test_sendto__validation_error(data, host, port, domain):
    sock = await aio_uring.prep_socket()

    with pytest.raises(expected_exception=TypeError):
        sock.sendto(data=data, host=host, port=port, domain=domain)

    await sock.close()


@aio_uring_test
async def test_sendto__no_req_params():
    sock = await aio_uring.prep_socket()

    with pytest.raises(expected_exception=TypeError):
        sock.sendto()

    await sock.close()


@aio_uring_test
async def test_sendto__closed_socket_raises_error():
    sock = await aio_uring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.sendto(data=b'hello', host='127.0.0.1', port=0, domain=socket.AF_INET)


@aio_uring_test
async def test_sendto__invalid_ip_raises_error():
    sock = await aio_uring.prep_socket()

    with pytest.raises(expected_exception=ConnectionRefusedError):
        sock.sendto(data=b'hello', host='not-an-ip-address', port=0, domain=socket.AF_INET)

    await sock.close()


@aio_uring_test
async def test_sendto__success():
    # UDP: no listener required for the send itself to succeed at the
    # io_uring/socket level (unlike TCP connect).
    server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server.bind(('127.0.0.1', 0))
    _, port = server.getsockname()

    sock = await aio_uring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)

    data = b'hello via sendto'
    written = await sock.sendto(data=data, host='127.0.0.1', port=port, domain=socket.AF_INET)
    assert written == len(data)

    received, _ = server.recvfrom(1024)
    assert received == data

    server.close()
    await sock.close()
