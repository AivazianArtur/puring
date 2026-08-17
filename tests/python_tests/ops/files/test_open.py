import os
import sys

sys.path.insert(0, '')

from pathlib import Path

import uringio
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import uringio_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'open_test_file.bin'
    path.write_bytes(b'\x00' * 100)
    return path


@pytest_parametrize(
    (
        'path',
        'dirfd',
        'flags',
        'resolve',
        'mode',
    ),
    (
        pytest_param(
            path=None,
            dirfd=0,
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=1,
            id='path_wrong_type',
        ),
        pytest_param(
            path=b'/example_path',
            dirfd=0,
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=1,
            id='path_bytes_type',
        ),
        pytest_param(
            path='',
            dirfd=None,
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=1,
            id='dirfd_wrong_type',
        ),
        pytest_param(
            path='',
            dirfd='not_an_int',
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=1,
            id='dirfd_wrong_type_str',
        ),
        pytest_param(
            path='',
            dirfd=0,
            flags=None,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=0,
            id='flags_wrong_type',
        ),
        pytest_param(
            path='',
            dirfd=0,
            flags=1.5,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=1,
            id='flags_wrong_type_float',
        ),
        pytest_param(
            path='',
            dirfd=0,
            flags=0,
            resolve=None,
            mode=0,
            id='resolve_wrong_type',
        ),
        pytest_param(
            path='',
            dirfd=0,
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=None,
            id='mode_wrong_type',
        ),
    ),
)
@uringio_test
async def test_open__validation_error(
    path,
    dirfd,
    flags,
    resolve,
    mode,
):
    with pytest.raises(TypeError):
        await uringio.open_file(
            path=path,
            dirfd=dirfd,
            flags=flags,
            resolve=resolve,
            mode=mode,
        )


@uringio_test
async def test_open__no_req_params():
    with pytest.raises(TypeError) as err:
        await uringio.open_file(
            dirfd=0,
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=0,
        )

    assert "function missing required argument 'path'" in str(err.value)


@pytest_parametrize(
    ('flags', 'resolve', 'mode'),
    (
        pytest_param(
            flags=None,
            resolve=None,
            mode=None,
            id='only_required_param',
        ),
        pytest_param(
            flags=0,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=0,
            id='resolve_default',
        ),
        pytest_param(
            flags=0,
            resolve=uringio.ResolveFlags.NO_MAGICLINKS,
            mode=0,
            id='resolve_flag_NO_MAGICLINKS',
        ),
        pytest_param(
            flags=0,
            resolve=uringio.ResolveFlags.NO_SYMLINKS,
            mode=0,
            id='resolve_flag_NO_SYMLINKS',
        ),
        pytest_param(
            flags=0,
            resolve=uringio.ResolveFlags.BENEATH,
            mode=0,
            id='resolve_flag_BENEATH',
        ),
        pytest_param(
            flags=0,
            resolve=uringio.ResolveFlags.IN_ROOT,
            mode=0,
            id='resolve_flag_IN_ROOT',
        ),
        pytest_param(
            flags=0,
            resolve=uringio.ResolveFlags.CACHED,
            mode=0,
            id='resolve_flag',
        ),
        pytest_param(
            flags=os.O_RDONLY,
            resolve=uringio.ResolveFlags.NO_XDEV,
            mode=0,
            id='realistic_flags',
        ),
        pytest_param(
            flags=os.O_RDONLY,
            resolve=uringio.ResolveFlags.NO_XDEV | uringio.ResolveFlags.CACHED,
            mode=0,
            id='resolve_flags_combined',
        ),
    ),
)
@uringio_test
async def test_open__success(
    temp_file_path,
    flags,
    resolve,
    mode,
):
    dirfd = None

    kwargs = {'path': str(temp_file_path)}

    if resolve is not None:
        dirfd = os.open(
            temp_file_path.parent,
            os.O_RDONLY | os.O_DIRECTORY,
        )

        kwargs['path'] = temp_file_path.name
        kwargs['dirfd'] = dirfd
        kwargs['resolve'] = resolve

    if flags is not None:
        kwargs['flags'] = flags

    if mode is not None:
        kwargs['mode'] = mode

    try:
        uring_file = await uringio.open_file(**kwargs)
        assert uring_file
        await uring_file.close()
    finally:
        if dirfd is not None:
            os.close(dirfd)

@pytest_parametrize(
    ('path'),
    (
        pytest_param(
            path='example_path/',
            id='path_str',
        ),
        pytest_param(
            path=Path('/example_path'),
            id='path_pathlib',
        ),
    ),
)
@uringio_test
async def test_open__path_types_success(temp_file_path, path):
    if isinstance(path, str):
        path = str(temp_file_path)

    elif isinstance(path, Path):
        path = temp_file_path

    uring_file = await uringio.open_file(
        path=path,
    )

    assert uring_file

    await uring_file.close()


@uringio_test
async def test_open__dirfd_negative_value(temp_file_path):
    uring_file = await uringio.open_file(
        path=temp_file_path,
        dirfd=-100,  # AT_FDCWD
    )

    assert uring_file
    await uring_file.close()
