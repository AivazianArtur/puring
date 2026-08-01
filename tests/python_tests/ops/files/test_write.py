import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'write_test_file.bin'
    path.write_bytes(b'\x00' * 1000)
    return str(path)


@pytest_parametrize(
    ('offset'),
    (
        pytest_param(offset='not_an_int', id='offset_wrong_type_str'),
        pytest_param(offset=None, id='offset_wrong_type_none'),
        pytest_param(offset=1.5, id='offset_wrong_type_float'),
    ),
)
@puring_test
async def test_write__validation_error(temp_file_path, offset):
    uring_file = await puring.open_file(path=temp_file_path, flags=0)

    with pytest.raises(expected_exception=TypeError):
        uring_file.write(data=b'hello', offset=offset)

    await uring_file.close()


@puring_test
async def test_write__closed_file_raises_error(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, flags=0)
    await uring_file.close()

    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.write(data=b'hello')


@puring_test
async def test_write__success(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, flags=0)

    data = b'hello world'
    written = await uring_file.write(data=data)
    assert written == len(data)

    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        content = f.read(len(data))
    assert content == data


@puring_test
async def test_write__with_offset(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, flags=0)

    data = b'PATCHED'
    written = await uring_file.write(data=data, offset=10)
    assert written == len(data)

    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        f.seek(10)
        content = f.read(len(data))
    assert content == data


@puring_test
async def test_write__bytearray_input(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, flags=0)

    data = bytearray(b'from bytearray')
    written = await uring_file.write(data=data)
    assert written == len(data)

    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        content = f.read(len(data))
    assert content == bytes(data)
