import sys

sys.path.insert(0, '')

import uringio
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import uringio_test


@pytest_parametrize(
    ('host', 'port'),
    (
        pytest_param(host=None, port=0, id='host_wrong_type'),
        pytest_param(host=1234, port=0, id='host_int_type'),
        pytest_param(host='127.0.0.1', port=None, id='port_wrong_type'),
        pytest_param(host='127.0.0.1', port='not_an_int', id='port_wrong_type_str'),
        pytest_param(host='127.0.0.1', port=1.5, id='port_wrong_type_float'),
    ),
)
@uringio_test
async def test_bind__validation_error(host, port):
    sock = await uringio.prep_socket()

    with pytest.raises(expected_exception=TypeError):
        sock.bind(host=host, port=port)

    await sock.close()


@uringio_test
async def test_bind__invalid_ip_raises_error():
    sock = await uringio.prep_socket()

    with pytest.raises(expected_exception=ConnectionRefusedError):
        sock.bind(host='not-an-ip-address', port=0)

    await sock.close()


@uringio_test
async def test_bind__closed_socket_raises_error():
    sock = await uringio.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.bind(host='127.0.0.1', port=0)


@uringio_test
async def test_bind__success():
    sock = await uringio.prep_socket()

    result = await sock.bind(host='127.0.0.1', port=0)
    assert result == 0

    await sock.close()


@uringio_test
async def test_bind__specific_port_success():
    import socket as stdlib_socket

    tmp = stdlib_socket.socket(stdlib_socket.AF_INET, stdlib_socket.SOCK_STREAM)
    tmp.bind(('127.0.0.1', 0))
    _, port = tmp.getsockname()
    tmp.close()

    sock = await uringio.prep_socket()
    result = await sock.bind(host='127.0.0.1', port=port)
    assert result == 0

    await sock.close()


@uringio_test
async def test_bind__double_bind_raises_error():
    sock = await uringio.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    with pytest.raises(expected_exception=OSError):
        await sock.bind(host='127.0.0.1', port=0)

    await sock.close()
