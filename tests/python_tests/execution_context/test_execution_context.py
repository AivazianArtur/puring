import sys

sys.path.insert(0, '')

import asyncio
import puring
import pytest

from tests.python_tests.tests_utils.runner import puring_test


@puring_test
async def test_execution_context__invalid_stream_strategy_raises_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(
            stream_strategy=99999,
            buffer_mode=puring.BUFFER_MODE.FIXED,
            transfer_mode=puring.TRANSFER_MODE.NORMAL,
            buffers=[buf],
        )


@puring_test
async def test_execution_context__invalid_transfer_mode_raises_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(
            stream_strategy=puring.STREAM_STRATEGY.ONESHOT,
            buffer_mode=puring.BUFFER_MODE.FIXED,
            transfer_mode=99999,
            buffers=[buf],
        )


@puring_test
async def test_execution_context__enter_exit_no_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with loop.execution_context(
        stream_strategy=puring.STREAM_STRATEGY.ONESHOT,
        buffer_mode=puring.BUFFER_MODE.FIXED,
        transfer_mode=puring.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        pass


@puring_test
async def test_execution_context__fixed_buffer_actually_used(tmp_path):
    path = tmp_path / 'execution_context_fixed.bin'
    path.write_bytes(b'hello execution context')

    buf = bytearray(64)

    uring_file = await puring.open_file(path=str(path))
    loop = asyncio.get_running_loop()

    with loop.execution_context(
        stream_strategy=puring.STREAM_STRATEGY.ONESHOT,
        buffer_mode=puring.BUFFER_MODE.FIXED,
        transfer_mode=puring.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        result = await uring_file.read(size=len(b'hello execution context'))

    assert result == b'hello execution context'

    await uring_file.close()
