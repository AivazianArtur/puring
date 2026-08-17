import sys

sys.path.insert(0, '')

import asyncio
import uringio
import pytest

from tests.python_tests.tests_utils.runner import uringio_test


@uringio_test
async def test_execution_context__invalid_stream_strategy_raises_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(
            stream_strategy=99999,
            buffer_mode=uringio.BUFFER_MODE.FIXED,
            transfer_mode=uringio.TRANSFER_MODE.NORMAL,
            buffers=[buf],
        )


@uringio_test
async def test_execution_context__invalid_transfer_mode_raises_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(
            stream_strategy=uringio.STREAM_STRATEGY.ONESHOT,
            buffer_mode=uringio.BUFFER_MODE.FIXED,
            transfer_mode=99999,
            buffers=[buf],
        )


@uringio_test
async def test_execution_context__enter_exit_no_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with loop.execution_context(
        stream_strategy=uringio.STREAM_STRATEGY.ONESHOT,
        buffer_mode=uringio.BUFFER_MODE.FIXED,
        transfer_mode=uringio.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        pass


@uringio_test
async def test_execution_context__fixed_buffer_actually_used(tmp_path):
    path = tmp_path / 'execution_context_fixed.bin'
    path.write_bytes(b'hello execution context')

    buf = bytearray(64)

    uring_file = await uringio.open_file(path=str(path))
    loop = asyncio.get_running_loop()

    with loop.execution_context(
        stream_strategy=uringio.STREAM_STRATEGY.ONESHOT,
        buffer_mode=uringio.BUFFER_MODE.FIXED,
        transfer_mode=uringio.TRANSFER_MODE.NORMAL,
        buffers=[buf],
    ):
        result = await uring_file.read(size=len(b'hello execution context'))

    assert result == b'hello execution context'

    await uring_file.close()
