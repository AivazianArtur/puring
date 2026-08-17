import ctypes
import sys

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import aio_uring_test


class _Iovec(ctypes.Structure):
    _fields_ = [
        ('iov_base', ctypes.c_void_p),
        ('iov_len', ctypes.c_size_t),
    ]


def _build_iovecs(buffers):
    """
    Build a raw iovecs buffer (bytearray of packed `struct iovec` entries)
    pointing at the given writable buffer-like objects.
    """
    iovecs = (_Iovec * len(buffers))()
    for i, buf in enumerate(buffers):
        c_buf = (ctypes.c_char * len(buf)).from_buffer(buf)
        iovecs[i].iov_base = ctypes.addressof(c_buf)
        iovecs[i].iov_len = len(buf)
    return bytearray(iovecs)


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'readv_raw_test_file.bin'
    path.write_bytes(b'0123456789' * 100)
    return str(path)


@pytest_parametrize(
    ('iovecs'),
    (
        pytest_param(
            iovecs=bytearray(1),
            id='iovecs_invalid_size_not_multiple_of_struct_iovec',
        ),
        pytest_param(
            iovecs=bytearray(ctypes.sizeof(_Iovec) - 1),
            id='iovecs_invalid_size_one_byte_short',
        ),
    ),
)
@aio_uring_test
async def test_readv_raw__validation_error(temp_file_path, iovecs):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    with pytest.raises(expected_exception=ValueError):
        uring_file.readv_raw(iovecs=iovecs)

    await uring_file.close()


@aio_uring_test
async def test_readv_raw__non_contiguous_buffer_raises_error(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    buf = bytearray(ctypes.sizeof(_Iovec) * 2)
    non_contiguous = memoryview(buf)[::2]

    with pytest.raises(expected_exception=ValueError):
        uring_file.readv_raw(iovecs=non_contiguous)

    await uring_file.close()


@aio_uring_test
async def test_readv_raw__closed_file_raises_error(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)
    await uring_file.close()

    buf = bytearray(16)
    iovecs = _build_iovecs([buf])

    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.readv_raw(iovecs=iovecs)


@pytest_parametrize(
    ('buffer_sizes', 'nowait'),
    (
        pytest_param(
            buffer_sizes=[16],
            nowait=None,
            id='single_buffer',
        ),
        pytest_param(
            buffer_sizes=[16, 32],
            nowait=None,
            id='multiple_buffers',
        ),
        pytest_param(
            buffer_sizes=[16],
            nowait=0,
            id='explicit_flags',
        ),
    ),
)
@aio_uring_test
async def test_readv_raw__success(temp_file_path, buffer_sizes, nowait):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    buffers = [bytearray(size) for size in buffer_sizes]
    iovecs = _build_iovecs(buffers)

    kwargs = {'iovecs': iovecs}
    if nowait is not None:
        kwargs['nowait'] = nowait

    assert await uring_file.readv_raw(**kwargs)

    await uring_file.close()


@aio_uring_test
async def test_readv_raw__full_roundtrip(temp_file_path):
    uring_file = await aio_uring.open_file(path=temp_file_path)

    buf1 = bytearray(500)
    buf2 = bytearray(500)
    iovecs = _build_iovecs([buf1, buf2])

    await uring_file.readv_raw(iovecs=iovecs)
    await uring_file.close()

    assert bytes(buf1) + bytes(buf2) == b'0123456789' * 100
