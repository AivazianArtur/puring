import asyncio
import sys
import time

sys.path.insert(0, '')

import aio_uring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import aio_uring_test


@pytest_parametrize(
    ('timer_params'),
    (
        pytest_param(timer_params='not_a_dict', id='timer_params_wrong_type_str'),
        pytest_param(timer_params=123, id='timer_params_wrong_type_int'),
        pytest_param(timer_params=[1, 2, 3], id='timer_params_wrong_type_list'),
    ),
)
@aio_uring_test
async def test_timer__validation_error(timer_params):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=TypeError):
        aio_uring.timer(loop, timer_params=timer_params)


@pytest_parametrize(
    ('timer_params'),
    (
        pytest_param(timer_params={'nsec': 0, 'count': 0}, id='missing_sec'),
        pytest_param(timer_params={'sec': 0, 'count': 0}, id='missing_nsec'),
        pytest_param(timer_params={'sec': 0, 'nsec': 0}, id='missing_count'),
        pytest_param(timer_params={'sec': 'x', 'nsec': 0, 'count': 0}, id='sec_wrong_type'),
        pytest_param(timer_params={'sec': 0, 'nsec': 'x', 'count': 0}, id='nsec_wrong_type'),
        pytest_param(timer_params={'sec': 0, 'nsec': 0, 'count': 'x'}, id='count_wrong_type'),
    ),
)
@aio_uring_test
async def test_timer__incomplete_params_raises_error(timer_params):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=TypeError):
        aio_uring.timer(loop, timer_params=timer_params)


@aio_uring_test
async def test_timer__no_params_success():
    loop = asyncio.get_running_loop()

    result = await aio_uring.timer(loop)
    assert result is None


@aio_uring_test
async def test_timer__zero_delay_success():
    loop = asyncio.get_running_loop()

    result = await aio_uring.timer(loop, timer_params={'sec': 0, 'nsec': 0, 'count': 0})
    assert result is None


@aio_uring_test
async def test_timer__actually_waits():
    loop = asyncio.get_running_loop()

    start = time.monotonic()
    await aio_uring.timer(loop, timer_params={'sec': 0, 'nsec': 100_000_000, 'count': 0})  # 100ms
    elapsed = time.monotonic() - start

    assert elapsed >= 0.08


@aio_uring_test
async def test_timer__does_not_raise_on_expiry():
    loop = asyncio.get_running_loop()

    try:
        result = await aio_uring.timer(loop, timer_params={'sec': 0, 'nsec': 50_000_000, 'count': 0})
    except OSError as exc:
        pytest.fail(f'timer raised OSError on normal expiry: {exc!r}')

    assert result is None


@aio_uring_test
async def test_timer__concurrent_timers():
    loop = asyncio.get_running_loop()

    start = time.monotonic()
    await asyncio.gather(
        aio_uring.timer(loop, timer_params={'sec': 0, 'nsec': 50_000_000, 'count': 0}),
        aio_uring.timer(loop, timer_params={'sec': 0, 'nsec': 100_000_000, 'count': 0}),
    )
    elapsed = time.monotonic() - start

    assert elapsed < 0.14
