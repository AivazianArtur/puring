import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.runner import aio_uring_test


@aio_uring_test
async def test_close__success():
    sock = await aio_uring.prep_socket()

    result = await sock.close()
    assert result == 0


@aio_uring_test
async def test_close__double_close_raises_error():
    sock = await aio_uring.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.close()


@aio_uring_test
async def test_close__after_bind():
    sock = await aio_uring.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    result = await sock.close()
    assert result == 0


@aio_uring_test
async def test_close__after_bind_and_listen():
    sock = await aio_uring.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)
    await sock.listen(backlog=1)

    result = await sock.close()
    assert result == 0
