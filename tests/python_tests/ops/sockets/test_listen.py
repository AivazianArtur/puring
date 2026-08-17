import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest_parametrize(
    ('backlog'),
    (
        pytest_param(backlog='not_an_int', id='backlog_wrong_type_str'),
        pytest_param(backlog=None, id='backlog_wrong_type_none'),
        pytest_param(backlog=1.5, id='backlog_wrong_type_float'),
    ),
)
@aio_uring_test
async def test_listen__validation_error(backlog):
    sock = await aio_uring.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    with pytest.raises(expected_exception=TypeError):
        sock.listen(backlog=backlog)

    await sock.close()


@aio_uring_test
async def test_listen__closed_socket_raises_error():
    sock = await aio_uring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.listen(backlog=1)


@aio_uring_test
async def test_listen__success():
    sock = await aio_uring.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    result = await sock.listen(backlog=1)
    assert result == 0

    await sock.close()


@aio_uring_test
async def test_listen__zero_backlog_success():
    sock = await aio_uring.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    result = await sock.listen(backlog=0)
    assert result == 0

    await sock.close()
