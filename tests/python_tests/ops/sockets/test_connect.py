import socket
import sys
import threading

sys.path.insert(0, '')

import uringio
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import uringio_test


@pytest.fixture
def listening_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', 0))
    server.listen(1)
    host, port = server.getsockname()

    def _accept_once():
        try:
            conn, _ = server.accept()
            conn.close()
        except OSError:
            pass

    thread = threading.Thread(target=_accept_once, daemon=True)
    thread.start()

    yield host, port

    server.close()
    thread.join(timeout=1)


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
async def test_connect__validation_error(host, port):
    sock = await uringio.prep_socket()

    with pytest.raises(expected_exception=TypeError):
        sock.connect(host=host, port=port)

    await sock.close()


@uringio_test
async def test_connect__invalid_ip_raises_error():
    sock = await uringio.prep_socket()

    with pytest.raises(expected_exception=ConnectionRefusedError):
        sock.connect(host='not-an-ip-address', port=0)

    await sock.close()


@uringio_test
async def test_connect__closed_socket_raises_error():
    sock = await uringio.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.connect(host='127.0.0.1', port=0)


@uringio_test
async def test_connect__refused_raises_oserror():
    sock = await uringio.prep_socket()

    with pytest.raises(expected_exception=OSError):
        await sock.connect(host='127.0.0.1', port=1)

    await sock.close()


@uringio_test
async def test_connect__success(listening_server):
    host, port = listening_server

    sock = await uringio.prep_socket()
    result = await sock.connect(host=host, port=port)
    assert result == 0

    await sock.close()
