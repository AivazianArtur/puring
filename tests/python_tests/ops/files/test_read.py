import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'read_test_file.bin'
    path.write_bytes(b'0123456789' * 100)
    return str(path)


@pytest_parametrize(
    ('offset', 'size'),
    (
        pytest_param(
            offset='not_an_int',
            size=1024,
            id='offset_wrong_type_str',
        ),
        pytest_param(
            offset=None,
            size=1024,
            id='offset_wrong_type_none',
        ),
        pytest_param(
            offset=1.5,
            size=1024,
            id='offset_wrong_type_float',
        ),
        pytest_param(
            offset=0,
            size='not_an_int',
            id='size_wrong_type_str',
        ),
        pytest_param(
            offset=0,
            size=None,
            id='size_wrong_type_none',
        ),
        pytest_param(
            offset=0,
            size=1.5,
            id='size_wrong_type_float',
        ),
    ),
)
@puring_test
async def test_read__validation_error(temp_file_path, offset, size):
    uring_file = await puring.open_file(path=temp_file_path)

    with pytest.raises(expected_exception=TypeError):
        await uring_file.read(
            offset=offset,
            size=size,
        )

    await uring_file.close()


@puring_test
async def test_read__closed_file_raises_error(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)
    await uring_file.close()
    with pytest.raises(expected_exception=BrokenPipeError):
        await uring_file.read()


@pytest_parametrize(
    ('offset', 'size'),
    (
        pytest_param(
            offset=None,
            size=None,
            id='all_defaults',
        ),
        pytest_param(
            offset=0,
            size=None,
            id='only_offset',
        ),
        pytest_param(
            offset=None,
            size=512,
            id='only_size',
        ),
        pytest_param(
            offset=0,
            size=256,
            id='offset_and_size',
        ),
        pytest_param(
            offset=0,
            size=1024,
            id='offset_and_big_size',
        ),
        pytest_param(
            offset=-1,
            size=1024,
            id='offset_negative_default_value',
        ),
        pytest_param(
            offset=0,
            size=0,
            id='size_zero',
        ),
    ),
)
@puring_test
async def test_read__success(temp_file_path, offset, size):
    uring_file = await puring.open_file(path=temp_file_path)

    kwargs = {}
    if offset is not None:
        kwargs['offset'] = offset
    if size is not None:
        kwargs['size'] = size

    assert uring_file.read(**kwargs)


@puring_test
async def test_read__full_roundtrip(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)
    data = await uring_file.read(size=1024)
    await uring_file.close()

    assert len(data) == len(b'0123456789' * 100)
