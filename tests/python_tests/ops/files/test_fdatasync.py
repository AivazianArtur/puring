import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'fdatasync_test.bin'
    path.write_bytes(b'test')
    return str(path)


@puring_test
async def test_fdatasync__success(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    assert uring_file.fdatasync()

    await uring_file.close()


@puring_test
async def test_fdatasync__after_write(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    await uring_file.write(data=b'abcdef')

    assert uring_file.fdatasync()

    await uring_file.close()


@puring_test
async def test_fdatasync__multiple_calls(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    assert await uring_file.fdatasync() == 0
    assert await uring_file.fdatasync() == 0

    await uring_file.close()


@puring_test
async def test_fdatasync__closed_file_raises_error(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    await uring_file.close()

    with pytest.raises(BrokenPipeError):
        uring_file.fdatasync()
