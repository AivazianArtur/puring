import sys

sys.path.insert(0, '')

import asyncio
import aio_uring
import pytest

from tests.python_tests.tests_utils.runner import aio_uring_test


@aio_uring_test
async def test_execution_context__invalid_stream_strategy_raises_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(
            stream_strategy=99999,
            buffer_mode=aio_uring.BUFFER_MODE.FIXED,
            transfer_mode=aio_uring.TRANSFER_MODE.NORMAL,
            buffers=[buf],
        )


@aio_uring_test
async def test_execution_context__invalid_transfer_mode_raises_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(
            stream_strategy=aio_uring.STREAM_STRATEGY.ONESHOT,
            buffer_mode=aio_uring.BUFFER_MODE.FIXED,
            transfer_mode=99999,
            buffers=[buf],
        )


@aio_uring_test
async def test_execution_context__enter_exit_no_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with loop.execution_context(
        stream_strategy=aio_uring.STREAM_STRATEGY.ONESHOT,
        buffer_mode=aio_uring.BUFFER_MODE.FIXED,
        transfer_mode=aio_uring.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        pass


@aio_uring_test
async def test_execution_context__fixed_buffer_actually_used(tmp_path):
    path = tmp_path / 'execution_context_fixed.bin'
    path.write_bytes(b'hello execution context')

    buf = bytearray(64)

    uring_file = await aio_uring.open_file(path=str(path))
    loop = asyncio.get_running_loop()

    with loop.execution_context(
        stream_strategy=aio_uring.STREAM_STRATEGY.ONESHOT,
        buffer_mode=aio_uring.BUFFER_MODE.FIXED,
        transfer_mode=aio_uring.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        result = await uring_file.read(size=len(b'hello execution context'))

    assert result == b'hello execution context'

    await uring_file.close()
