import ctypes
import sys

sys.path.insert(0, '')

import uringio
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import uringio_test


class _Iovec(ctypes.Structure):
    _fields_ = [
        ('iov_base', ctypes.c_void_p),
        ('iov_len', ctypes.c_size_t),
    ]


def _build_iovecs(buffers):
    """
    Build a raw iovecs buffer (bytearray of packed `struct iovec` entries)
    pointing at the given buffer-like objects. Buffers must be kept alive
    by the caller for the duration of the syscall (data is not copied).
    """
    iovecs = (_Iovec * len(buffers))()
    for i, buf in enumerate(buffers):
        c_buf = (ctypes.c_char * len(buf)).from_buffer(buf)
        iovecs[i].iov_base = ctypes.addressof(c_buf)
        iovecs[i].iov_len = len(buf)
    return bytearray(iovecs)


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'writev_raw_test_file.bin'
    path.write_bytes(b'\x00' * 1000)
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
@uringio_test
async def test_writev_raw__validation_error(temp_file_path, iovecs):
    uring_file = await uringio.open_file(path=temp_file_path, flags=-1)

    with pytest.raises(expected_exception=ValueError):
        uring_file.writev_raw(buffers=iovecs)

    await uring_file.close()


@uringio_test
async def test_writev_raw__non_contiguous_buffer_raises_error(temp_file_path):
    uring_file = await uringio.open_file(path=temp_file_path, flags=-1)

    buf = bytearray(ctypes.sizeof(_Iovec) * 2)
    non_contiguous = memoryview(buf)[::2]

    with pytest.raises(expected_exception=BufferError):
        uring_file.writev_raw(buffers=non_contiguous)

    await uring_file.close()


@uringio_test
async def test_writev_raw__offset_accepts_int(temp_file_path):
    uring_file = await uringio.open_file(path=temp_file_path, flags=-1)

    buf = bytearray(b'0123456789abcdef')
    iovecs = _build_iovecs([buf])

    assert await uring_file.writev_raw(buffers=iovecs, offset=0)

    await uring_file.close()


@uringio_test
async def test_writev_raw__closed_file_raises_error(temp_file_path):
    uring_file = await uringio.open_file(path=temp_file_path, flags=-1)
    await uring_file.close()

    buf = bytearray(b'hello')
    iovecs = _build_iovecs([buf])

    with pytest.raises(expected_exception=BrokenPipeError):
        uring_file.writev_raw(buffers=iovecs)


@pytest_parametrize(
    ('chunks', 'flags'),
    (
        pytest_param(
            chunks=[bytearray(b'hello world!')],
            flags=None,
            id='single_buffer',
        ),
        pytest_param(
            chunks=[bytearray(b'hello '), bytearray(b'world!')],
            flags=None,
            id='multiple_buffers',
        ),
        pytest_param(
            chunks=[bytearray(b'hello world!')],
            flags=0,
            id='explicit_flags',
        ),
    ),
)
@uringio_test
async def test_writev_raw__success(temp_file_path, chunks, flags):
    uring_file = await uringio.open_file(path=temp_file_path, flags=-1)

    iovecs = _build_iovecs(chunks)
    total_len = sum(len(c) for c in chunks)

    kwargs = {'buffers': iovecs}
    if flags is not None:
        kwargs['flags'] = flags

    assert await uring_file.writev_raw(**kwargs)

    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        content = f.read(total_len)
    assert content == b''.join(bytes(c) for c in chunks)


@uringio_test
async def test_writev_raw__full_roundtrip(temp_file_path):
    uring_file = await uringio.open_file(path=temp_file_path, flags=-1)

    buf1 = bytearray(b'A' * 500)
    buf2 = bytearray(b'B' * 500)
    iovecs = _build_iovecs([buf1, buf2])

    await uring_file.writev_raw(buffers=iovecs)
    await uring_file.close()

    with open(temp_file_path, 'rb') as f:
        content = f.read(1000)
    assert content == bytes(buf1) + bytes(buf2)
