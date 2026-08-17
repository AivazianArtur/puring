import sys

sys.path.insert(0, '')

import asyncio
import uringio
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import uringio_test


@pytest_parametrize(
    ('mode'),
    (
        pytest_param(mode='not_an_int', id='mode_wrong_type_str'),
        pytest_param(mode=None, id='mode_wrong_type_none'),
        pytest_param(mode=1.5, id='mode_wrong_type_float'),
    ),
)
@uringio_test
async def test_buffer_mode__validation_error(mode):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=TypeError):
        loop.buffer_mode(mode=mode)


@uringio_test
async def test_buffer_mode__invalid_enum_value_raises_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.buffer_mode(mode=99999)


@uringio_test
async def test_buffer_mode__enter_exit_no_error():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with loop.buffer_mode(mode=uringio.BUFFER_MODE.FIXED, buffers=[buf]):
        pass


@uringio_test
async def test_buffer_mode__exception_inside_propagates():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)

    with pytest.raises(expected_exception=RuntimeError, match='boom'):
        with loop.buffer_mode(mode=uringio.BUFFER_MODE.FIXED, buffers=[buf]):
            raise RuntimeError('boom')


@uringio_test
async def test_buffer_mode__nested_fixed_raises_and_outer_still_valid(tmp_path):
    path = tmp_path / 'nested_buffer_mode.bin'
    path.write_bytes(b'0123456789' * 10)

    loop = asyncio.get_running_loop()
    outer_buf = bytearray(64)
    inner_buf = bytearray(64)

    uring_file = await uringio.open_file(path=str(path))

    with loop.buffer_mode(
        mode=uringio.BUFFER_MODE.FIXED, payload_type=uringio.PAYLOAD_TYPE.IOVEC, buffers=[outer_buf]
    ):
        with pytest.raises(expected_exception=RuntimeWarning):
            with loop.buffer_mode(
                mode=uringio.BUFFER_MODE.FIXED, payload_type=uringio.PAYLOAD_TYPE.IOVEC, buffers=[inner_buf]
            ):
                pass

        result = await uring_file.read(size=10)
        assert result == b'0123456789'

    await uring_file.close()


@uringio_test
async def test_buffer_mode__fixed_buffer_actually_used(tmp_path):
    path = tmp_path / 'buffer_mode_fixed.bin'
    path.write_bytes(b'hello fixed buffer')

    buf = bytearray(64)

    uring_file = await uringio.open_file(path=str(path))
    loop = asyncio.get_running_loop()

    with loop.buffer_mode(mode=uringio.BUFFER_MODE.FIXED, buffers=[buf]):
        result = await uring_file.read(size=len(b'hello fixed buffer'))

    assert result == b'hello fixed buffer'

    await uring_file.close()
