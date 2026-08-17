import asyncio
import sys

sys.path.insert(0, '')

import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import uringio_test

NORMAL_BUF = 0
BUF_NO_VAL = 4

PAYLOAD_LINEAR = 0
PAYLOAD_TYPE_NO_VAL = 3

ONESHOT = 0
MULTISHOT = 1

NORMAL_TRANSFER = 0
ZERO_COPY = 1

@uringio_test
async def test_loop_close__raises_while_running():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=RuntimeError, match='running'):
        loop.close()


@pytest_parametrize(
    ('mode'),
    (
        pytest_param(mode=BUF_NO_VAL, id='buf_no_val'),
        pytest_param(mode=999, id='out_of_range'),
    ),
)
@uringio_test
async def test_loop_buffer_mode__invalid_mode_raises_value_error(mode):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.buffer_mode(mode=mode)


@pytest_parametrize(
    ('payload_type'),
    (
        pytest_param(payload_type=PAYLOAD_TYPE_NO_VAL, id='payload_type_no_val'),
        pytest_param(payload_type=999, id='out_of_range'),
    ),
)
@uringio_test
async def test_loop_buffer_mode__invalid_payload_type_raises_value_error(payload_type):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.buffer_mode(mode=NORMAL_BUF, payload_type=payload_type)


@uringio_test
async def test_loop_buffer_mode__default_normal_buf_is_noop_context():
    loop = asyncio.get_running_loop()

    with loop.buffer_mode() as ctx:
        assert ctx is not None


@uringio_test
async def test_loop_buffer_mode__explicit_normal_buf_and_payload_type():
    loop = asyncio.get_running_loop()

    with loop.buffer_mode(mode=NORMAL_BUF, payload_type=PAYLOAD_LINEAR, amount=3, bufsize=1024):
        pass


@uringio_test
async def test_loop_execution_context__invalid_buffer_mode_raises_value_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(buffer_mode=BUF_NO_VAL)


@uringio_test
async def test_loop_write_to_self__does_not_raise():
    loop = asyncio.get_running_loop()

    result = loop._write_to_self()
    assert result is None


@pytest_parametrize(
    ('stream'),
    (pytest_param(stream=999, id='out_of_range'),),
)
@uringio_test
async def test_loop_stream_strategy__invalid_raises_value_error(stream):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.stream_strategy(stream=stream)


@pytest_parametrize(
    ('stream'),
    (
        pytest_param(stream=ONESHOT, id='oneshot'),
        pytest_param(stream=MULTISHOT, id='multishot'),
    ),
)
@uringio_test
async def test_loop_stream_strategy__valid_values_enter_context(stream):
    loop = asyncio.get_running_loop()

    with loop.stream_strategy(stream=stream) as ctx:
        assert ctx is not None


@uringio_test
async def test_loop_stream_strategy__default_is_oneshot():
    loop = asyncio.get_running_loop()

    with loop.stream_strategy() as ctx:
        assert ctx is not None


@pytest_parametrize(
    ('mode'),
    (pytest_param(mode=999, id='out_of_range'),),
)
@uringio_test
async def test_loop_transfer_mode__invalid_raises_value_error(mode):
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.transfer_mode(mode=mode)


@pytest_parametrize(
    ('mode'),
    (
        pytest_param(mode=NORMAL_TRANSFER, id='normal_transfer'),
        pytest_param(mode=ZERO_COPY, id='zero_copy'),
    ),
)
@uringio_test
async def test_loop_transfer_mode__valid_values_enter_context(mode):
    loop = asyncio.get_running_loop()

    with loop.transfer_mode(mode=mode) as ctx:
        assert ctx is not None


@uringio_test
async def test_loop_transfer_mode__default_is_normal_transfer():
    loop = asyncio.get_running_loop()

    with loop.transfer_mode() as ctx:
        assert ctx is not None


@uringio_test
async def test_loop_stream_strategy__nested_with_transfer_mode():
    loop = asyncio.get_running_loop()

    with loop.stream_strategy(stream=MULTISHOT):
        with loop.transfer_mode(mode=ZERO_COPY):
            pass


@uringio_test
async def test_loop_execution_context__invalid_stream_strategy_raises_value_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(stream_strategy=999)


@uringio_test
async def test_loop_execution_context__invalid_transfer_mode_raises_value_error():
    loop = asyncio.get_running_loop()

    with pytest.raises(expected_exception=ValueError):
        loop.execution_context(transfer_mode=999)
