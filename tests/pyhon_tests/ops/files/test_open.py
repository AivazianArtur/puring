import os
import sys

sys.path.insert(0, '')

import puring
import pytest

from pathlib import Path
from tests.pyhon_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.pyhon_tests.tests_utils.runner import puring_test

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
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=1,
            id='path_wrong_type',
        ),
        pytest_param(
            path=b'/example_path',
            dirfd=0,
            flags=0,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=1,
            id='path_bytes_type',
        ),
        pytest_param(
            path='',
            dirfd=None,
            flags=0,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=1,
            id='dirfd_wrong_type',
        ),
        pytest_param(
            path='',
            dirfd='not_an_int',
            flags=0,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=1,
            id='dirfd_wrong_type_str',
        ),
        pytest_param(
            path='',
            dirfd=0,
            flags=None,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=0,
            id='flags_wrong_type',
        ),
        pytest_param(
            path='',
            dirfd=0,
            flags=1.5,
            resolve=puring.ResolveFlags.NO_XDEV,
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
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=None,
            id='mode_wrong_type',
        ),
    ),
)
@puring_test
async def test_open__validation_error(
    path,
    dirfd,
    flags,
    resolve,
    mode,
):
    with pytest.raises(expected_exception=TypeError):
        puring.open_file(
            path=path,
            dirfd=dirfd,
            flags=flags,
            resolve=resolve,
            mode=mode,
        )


@puring_test
async def test_open__no_req_params():
    with pytest.raises(expected_exception=TypeError) as err:
        puring.open_file(
            dirfd=0,
            flags=0,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=0,
        )
        assert "function missing required argument 'path'" in err.match


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
            path='example_path/',
            dirfd=0,
            flags=0,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=1,
            id='path_str',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=0,
            flags=0,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=1,
            id='path_pathlib',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=None,
            mode=None,
            id='only_required_param',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=0,
            flags=None,
            resolve=None,
            mode=None,
            id='only_dirfd',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=0,
            resolve=None,
            mode=None,
            id='only_flags',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=None,
            id='only_resolve',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=0,
            flags=None,
            resolve=None,
            mode=0,
            id='only_mode',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.NO_MAGICLINKS,
            mode=None,
            id='resolve_flag_NO_MAGICLINKS',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.NO_SYMLINKS,
            mode=None,
            id='resolve_flag_NO_SYMLINKS',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.BENEATH,
            mode=None,
            id='resolve_flag_BENEATH',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.IN_ROOT,
            mode=None,
            id='resolve_flag_IN_ROOT',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.CACHED,
            mode=None,
            id='resolve_flag',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=0,
            flags=os.O_CREAT | os.O_RDWR,
            resolve=puring.ResolveFlags.NO_XDEV,
            mode=0o644,
            id='realistic_flags_and_octal_mode',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=None,
            flags=None,
            resolve=puring.ResolveFlags.NO_XDEV | puring.ResolveFlags.CACHED,
            mode=None,
            id='resolve_flags_combined',
        ),
        pytest_param(
            path=Path('/example_path'),
            dirfd=-100,  # AT_FDCWD
            flags=None,
            resolve=None,
            mode=None,
            id='dirfd_negative_value',
        ),

    ),
)
@puring_test
async def test_open__success(
    path,
    dirfd,
    flags,
    resolve,
    mode,
):
    args = [
        arg for arg in [path, dirfd, flags, resolve, mode] if arg is not None
    ]
    assert puring.open_file(*args)


@puring_test
async def test_open__resolve_flag_out_of_enum_range_currently_succeeds():
    # It will break once we will add proper validation
    assert puring.open_file(
        path='/example_path',
        resolve=99999,
    )
