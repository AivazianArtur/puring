import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'fsync_test.bin'
    path.write_bytes(b'test')
    return str(path)


@aio_uring_test
async def test_fsync__success(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    assert await uring_file.fsync() == 0

    await uring_file.close() == 0


@aio_uring_test
async def test_fsync__after_write(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    await uring_file.write(data=b'abcdef')

    assert await uring_file.fsync() == 0

    await uring_file.close() == 0


@aio_uring_test
async def test_fsync__multiple_calls(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    assert await uring_file.fsync() == 0
    assert await uring_file.fsync() == 0
    assert await uring_file.fsync() == 0

    await uring_file.close() == 0


@aio_uring_test
async def test_fsync__closed_file_raises_error(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    await uring_file.close()

    with pytest.raises(BrokenPipeError):
        uring_file.fsync()
