import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'splice_test.bin'
    path.write_bytes(b'')
    return str(path)


@pytest_parametrize(
    ('src', 'dst', 'count'),
    (
        pytest_param(src='1', dst=1, count=1, id='src_str'),
        pytest_param(src=1, dst='1', count=1, id='dst_str'),
        pytest_param(src=1, dst=1, count='1', id='count_str'),
        pytest_param(src=None, dst=1, count=1, id='src_none'),
        pytest_param(src=1, dst=None, count=1, id='dst_none'),
        pytest_param(src=1, dst=1, count=None, id='count_none'),
    ),
)
@aio_uring_test
async def test_splice__validation_error(
    temp_file_path,
    src,
    dst,
    count,
):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    with pytest.raises(TypeError):
        uring_file.splice(
            src=src,
            dst=dst,
            count=count,
        )

    await uring_file.close()


@pytest_parametrize(
    ('offset_src', 'offset_dst', 'flag'),
    (
        pytest_param(offset_src='1', offset_dst=0, flag=0, id='offset_src_str'),
        pytest_param(offset_src=0, offset_dst='', flag=0, id='offset_dst_str'),
        pytest_param(offset_src=0, offset_dst=0, flag='', id='flag_str'),
        pytest_param(offset_src=None, offset_dst=0, flag=0, id='offset_src_none'),
        pytest_param(offset_src=0, offset_dst=None, flag=0, id='offset_dst_none'),
        pytest_param(offset_src=0, offset_dst=0, flag=None, id='flag_none'),
    ),
)
@aio_uring_test
async def test_splice__optional_args_validation(
    temp_file_path,
    offset_src,
    offset_dst,
    flag,
):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    with pytest.raises(TypeError):
        uring_file.splice(
            src=0,
            dst=1,
            count=1,
            offset_src=offset_src,
            offset_dst=offset_dst,
            flag=flag,
        )

    await uring_file.close()


@aio_uring_test
async def test_splice__closed_file_raises_error(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    await uring_file.close()

    with pytest.raises(BrokenPipeError):
        uring_file.splice(
            src=0,
            dst=1,
            count=1,
        )


@aio_uring_test
async def test_splice__accepts_all_optional_arguments(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    assert uring_file.splice(
        src=0,
        dst=1,
        count=1,
        offset_src=0,
        offset_dst=0,
        flag=0,
    )

    await uring_file.close()
