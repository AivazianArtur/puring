import sys

sys.path.insert(0, '')

import asyncio
import puring
import pytest

from tests.python_tests.tests_utils.runner import puring_test


@puring_test
async def test_transfer_mode__invalid_enum_value_raises_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.transfer_mode(mode=99999)


@puring_test
async def test_transfer_mode__enter_exit_no_error():
    loop = asyncio.get_running_loop()

    with loop.transfer_mode(mode=puring.TRANSFER_MODE.NORMAL):
        pass


@puring_test
async def test_transfer_mode__exception_inside_propagates():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=RuntimeError, match='boom'):
        with loop.transfer_mode(mode=puring.TRANSFER_MODE.NORMAL):
            raise RuntimeError('boom')
