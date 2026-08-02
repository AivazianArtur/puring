import sys

sys.path.insert(0, '')

import asyncio
import puring
import pytest

from tests.python_tests.tests_utils.runner import puring_test


@puring_test
async def test_stream_strategy__invalid_enum_value_raises_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.stream_strategy(stream=99999)


@puring_test
async def test_stream_strategy__enter_exit_no_error():
    loop = asyncio.get_running_loop()

    with loop.stream_strategy(stream=puring.STREAM_STRATEGY.ONESHOT):
        pass


@puring_test
async def test_stream_strategy__exception_inside_propagates():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=RuntimeError, match='boom'):
        with loop.stream_strategy(stream=puring.STREAM_STRATEGY.ONESHOT):
            raise RuntimeError('boom')
