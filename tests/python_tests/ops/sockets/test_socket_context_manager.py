import socket
import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.runner import puring_test


@puring_test
async def test_socket_context_manager__closes_on_normal_exit():
    sock = await puring.prep_socket(domain=socket.AF_INET)

    async with sock as ctx_sock:
        assert ctx_sock is sock
        await ctx_sock.bind(host='127.0.0.1', port=0)

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.listen(backlog=1)


@puring_test
async def test_socket_context_manager__closes_on_exception():
    sock = await puring.prep_socket(domain=socket.AF_INET)

    with pytest.raises(expected_exception=ValueError):
        async with sock:
            raise ValueError('boom')

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.listen(backlog=1)


@puring_test
async def test_socket_context_manager__original_exception_propagates():
    sock = await puring.prep_socket(domain=socket.AF_INET)

    with pytest.raises(expected_exception=ValueError, match='specific error'):
        async with sock:
            raise ValueError('specific error')


@puring_test
async def test_socket_context_manager__aenter_returns_self():
    sock = await puring.prep_socket(domain=socket.AF_INET)

    result = await sock.__aenter__()
    assert result is sock

    await sock.close()


@puring_test
async def test_socket_context_manager__usable_for_bind_listen():
    async with (await puring.prep_socket(domain=socket.AF_INET)) as sock:
        await sock.bind(host='127.0.0.1', port=0)
        await sock.listen(backlog=1)

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.listen(backlog=1)
