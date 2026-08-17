import sys

sys.path.insert(0, '')

import asyncio
import aio_uring
import pytest

from tests.python_tests.tests_utils.runner import aio_uring_test


@aio_uring_test
async def test_transfer_mode__invalid_enum_value_raises_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.transfer_mode(mode=99999)


@aio_uring_test
async def test_transfer_mode__enter_exit_no_error():
    loop = asyncio.get_running_loop()

    with loop.transfer_mode(mode=aio_uring.TRANSFER_MODE.NORMAL):
        pass


@aio_uring_test
async def test_transfer_mode__exception_inside_propagates():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=RuntimeError, match='boom'):
        with loop.transfer_mode(mode=aio_uring.TRANSFER_MODE.NORMAL):
            raise RuntimeError('boom')
