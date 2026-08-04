import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'close_test.bin'
    path.write_bytes(b'')
    return str(path)


@puring_test
async def test_close__success(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    assert await uring_file.close() == 0


@puring_test
async def test_close__double_close(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    await uring_file.close()

    with pytest.raises(BrokenPipeError):
        uring_file.close()


@puring_test
async def test_close__after_write(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    await uring_file.write(data=b'hello')

    assert await uring_file.close() == 0


@puring_test
async def test_close__after_read(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    await uring_file.read()

    assert await uring_file.close() == 0
