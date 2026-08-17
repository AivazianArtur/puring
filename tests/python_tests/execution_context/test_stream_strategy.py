import sys

sys.path.insert(0, '')

import asyncio
import aio_uring
import pytest

from tests.python_tests.tests_utils.runner import aio_uring_test


@aio_uring_test
async def test_stream_strategy__invalid_enum_value_raises_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.stream_strategy(stream=99999)


@aio_uring_test
async def test_stream_strategy__enter_exit_no_error():
    loop = asyncio.get_running_loop()

    with loop.stream_strategy(stream=aio_uring.STREAM_STRATEGY.ONESHOT):
        pass


@aio_uring_test
async def test_stream_strategy__exception_inside_propagates():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=RuntimeError, match='boom'):
        with loop.stream_strategy(stream=aio_uring.STREAM_STRATEGY.ONESHOT):
            raise RuntimeError('boom')
