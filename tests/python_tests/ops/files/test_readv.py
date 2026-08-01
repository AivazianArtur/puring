import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'readv_test_file.bin'
    path.write_bytes(b'0123456789' * 100)
    return str(path)


@pytest_parametrize(
    ('offset',),
    (
        pytest_param(
            offset='not_an_int',
            id='offset_wrong_type_str',
        ),
        pytest_param(
            offset=None,
            id='offset_wrong_type_none',
        ),
        pytest_param(
            offset=1.5,
            id='offset_wrong_type_float',
        ),
    ),
)
@puring_test
async def test_readv__validation_error(temp_file_path, offset):
    uring_file = await puring.open_file(path=temp_file_path)

    with pytest.raises(expected_exception=TypeError):
        uring_file.readv(
            buffers=[bytearray(16)],
            offset=offset,
        )

    await uring_file.close()


@puring_test
async def test_readv__closed_file_raises_error(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)
    await uring_file.close()
    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.readv(buffers=[bytearray(16)])


@pytest_parametrize(
    (
        'buffers',
        'offset',
        'nowait',
    ),
    (
        pytest_param(
            buffers=None,
            offset=None,
            nowait=None,
            id='all_defaults',
        ),
        pytest_param(
            buffers=[bytearray(16)],
            offset=None,
            nowait=None,
            id='single_buffer',
        ),
        pytest_param(
            buffers=[bytearray(16), bytearray(32)],
            offset=None,
            nowait=None,
            id='multiple_buffers',
        ),
        pytest_param(
            buffers=[bytearray(16)],
            offset=0,
            nowait=None,
            id='with_offset',
        ),
        pytest_param(
            buffers=[bytearray(16)],
            offset=None,
            nowait=True,
            id='nowait_true',
        ),
        pytest_param(
            buffers=[bytearray(16)],
            offset=None,
            nowait=False,
            id='nowait_false',
        ),
        pytest_param(
            buffers=[bytearray(16)],
            offset=-1,
            nowait=None,
            id='offset_negative_default_value',
        ),
    ),
)
@puring_test
async def test_readv__success(temp_file_path, buffers, offset, nowait):
    uring_file = await puring.open_file(path=temp_file_path)
    kwargs = {}
    if buffers is not None:
        kwargs['buffers'] = buffers
    if offset is not None:
        kwargs['offset'] = offset
    if nowait is not None:
        kwargs['nowait'] = nowait

    assert uring_file.readv(**kwargs)

    await uring_file.close()


@puring_test
async def test_readv__full_roundtrip(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)

    buf1 = bytearray(500)
    buf2 = bytearray(500)
    await uring_file.readv(buffers=[buf1, buf2])
    await uring_file.close()

    assert bytes(buf1) + bytes(buf2) == b'0123456789' * 100
