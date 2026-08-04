import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'writev_test_file.bin'
    path.write_bytes(b'\x00' * 1000)
    return str(path)


@pytest_parametrize(
    ('buffers'),
    (
        pytest_param(buffers=123, id='buffers_wrong_type_int'),
        pytest_param(buffers=None, id='buffers_wrong_type_none'),
    ),
)
@puring_test
async def test_writev__validation_error(temp_file_path, buffers):
    uring_file = await puring.open_file(path=temp_file_path, flags=-1)

    with pytest.raises(expected_exception=TypeError):
        uring_file.writev(buffers=buffers)

    await uring_file.close()


@puring_test
async def test_writev__closed_file_raises_error(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, flags=-1)
    await uring_file.close()

    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.writev(buffers=[b'hello'])


@pytest_parametrize(
    ('chunks'),
    (
        pytest_param(chunks=[b'hello world'], id='single_buffer'),
        pytest_param(chunks=[b'hello ', b'world', b'!'], id='multiple_buffers'),
    ),
)
@puring_test
async def test_writev__success(temp_file_path, chunks):
    uring_file = await puring.open_file(path=temp_file_path, flags=-1)

    total_len = sum(len(c) for c in chunks)
    written = await uring_file.writev(buffers=chunks)
    assert written == total_len

    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        content = f.read(total_len)
    assert content == b''.join(chunks)


@puring_test
async def test_writev__with_offset(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, flags=-1)

    chunks = [b'AAA', b'BBB']
    written = await uring_file.writev(buffers=chunks, offset=20)
    assert written == 6

    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        f.seek(20)
        content = f.read(6)
    assert content == b'AAABBB'
