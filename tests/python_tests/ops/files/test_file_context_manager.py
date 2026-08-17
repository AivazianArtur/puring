import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'context_manager_test_file.bin'
    path.write_bytes(b'0123456789' * 10)
    return str(path)


@aio_uring_test
async def test_file_context_manager__closes_on_normal_exit(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    async with uring_file as ctx_file:
        assert ctx_file is uring_file
        data = await ctx_file.readv(buffers=[bytearray(16)])
        assert data

    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.readv(buffers=[bytearray(16)])


@aio_uring_test
async def test_file_context_manager__closes_on_exception(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    with pytest.raises(expected_exception=ValueError):
        async with uring_file:
            raise ValueError('boom')

    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.readv(buffers=[bytearray(16)])


@aio_uring_test
async def test_file_context_manager__original_exception_propagates(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    with pytest.raises(expected_exception=ValueError, match='specific error'):
        async with uring_file:
            raise ValueError('specific error')


@aio_uring_test
async def test_file_context_manager__aenter_returns_self_immediately(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    result = await uring_file.__aenter__()
    assert result is uring_file

    await uring_file.close()


@aio_uring_test
async def test_file_context_manager__usable_for_read(temp_file_path):
    async with (await aio_uring.open_file(path=temp_file_path)) as f:
        buf = bytearray(10)
        await f.readv(buffers=[buf])
        assert bytes(buf) == b'0123456789'

    with pytest.raises(expected_exception=BrokenPipeError):
        f.readv(buffers=[bytearray(10)])
