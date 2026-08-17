import sys

sys.path.insert(0, '')

import asyncio
import uringio
import pytest

from tests.python_tests.tests_utils.runner import uringio_test


@uringio_test
async def test_transfer_mode__invalid_enum_value_raises_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.transfer_mode(mode=99999)


@uringio_test
async def test_transfer_mode__enter_exit_no_error():
    loop = asyncio.get_running_loop()

    with loop.transfer_mode(mode=uringio.TRANSFER_MODE.NORMAL):
        pass


@uringio_test
async def test_transfer_mode__exception_inside_propagates():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=RuntimeError, match='boom'):
        with loop.transfer_mode(mode=uringio.TRANSFER_MODE.NORMAL):
            raise RuntimeError('boom')
