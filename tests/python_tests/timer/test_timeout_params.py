import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test


@pytest.fixture
def temp_file_path(tmp_path):
    path = tmp_path / 'timeout_params_test.bin'
    path.write_bytes(b'hello')
    return str(path)


@puring_test
async def test_timeout_params__none_is_valid_default(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path, timeout_params=None)
    assert uring_file is not None

    await uring_file.close()


@puring_test
async def test_timeout_params__omitted_is_valid_default(temp_file_path):
    uring_file = await puring.open_file(path=temp_file_path)
    assert uring_file is not None

    await uring_file.close()


@pytest_parametrize(
    ('timeout_params'),
    (
        pytest_param(timeout_params=123, id='int'),
        pytest_param(timeout_params='not_a_dict', id='str'),
        pytest_param(timeout_params=[1, 2, 3], id='list'),
    ),
)
@puring_test
async def test_timeout_params__non_dict_raises_type_error(temp_file_path, timeout_params):
    with pytest.raises(expected_exception=TypeError):
        await puring.open_file(path=temp_file_path, timeout_params=timeout_params)


@puring_test
async def test_timeout_params__non_dict_does_not_corrupt_interpreter_state(temp_file_path):
    try:
        maybe_future = puring.open_file(path=temp_file_path, timeout_params=123)
        if maybe_future is not None:
            try:
                await maybe_future
            except Exception:
                pass
    except TypeError:
        pass

    uring_file = await puring.open_file(path=temp_file_path)
    assert uring_file is not None
    await uring_file.close()


@pytest_parametrize(
    ('timeout_params'),
    (
        pytest_param(timeout_params={}, id='empty_dict'),
        pytest_param(timeout_params={'sec': 1}, id='missing_nsec_and_is_required'),
        pytest_param(timeout_params={'sec': 1, 'nsec': 0}, id='missing_is_required'),
        pytest_param(timeout_params={'nsec': 0, 'is_required': False}, id='missing_sec'),
        pytest_param(timeout_params={'sec': 1, 'nsec': 0, 'is_required': 'not_a_bool'}, id='is_required_wrong_type'),
        pytest_param(timeout_params={'sec': 'x', 'nsec': 0, 'is_required': False}, id='sec_wrong_type'),
        pytest_param(timeout_params={'sec': 1, 'nsec': 'x', 'is_required': False}, id='nsec_wrong_type'),
    ),
)
@puring_test
async def test_timeout_params__incomplete_or_malformed_dict_raises_type_error(temp_file_path, timeout_params):
    with pytest.raises(expected_exception=TypeError):
        await puring.open_file(path=temp_file_path, timeout_params=timeout_params)


@puring_test
async def test_timeout_params__zero_timeout_not_required_still_succeeds(temp_file_path):
    uring_file = await puring.open_file(
        path=temp_file_path,
        timeout_params={'sec': 0, 'nsec': 0, 'is_required': False},
    )
    assert uring_file is not None

    await uring_file.close()

@puring_test
@pytest.mark.skip
async def test_timeout_params__well_formed_dict_is_accepted(temp_file_path):
    # Skipping for now because we dont work with linked sqe in reader
    uring_file_future = puring.open_file(
        path=temp_file_path,
        timeout_params={'sec': 5, 'nsec': 1, 'is_required': False},
    )
    uring_file = await uring_file_future
    assert uring_file is not None

    await uring_file.close()
