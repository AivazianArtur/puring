import socket
import sys

sys.path.insert(0, '')

import uringio
import pytest

from tests.python_tests.tests_utils.runner import uringio_test


@uringio_test
async def test_socket_context_manager__closes_on_normal_exit():
    sock = await uringio.prep_socket(domain=socket.AF_INET)

    async with sock as ctx_sock:
        assert ctx_sock is sock
        await ctx_sock.bind(host='127.0.0.1', port=0)

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.listen(backlog=1)


@uringio_test
async def test_socket_context_manager__closes_on_exception():
    sock = await uringio.prep_socket(domain=socket.AF_INET)

    with pytest.raises(expected_exception=ValueError):
        async with sock:
            raise ValueError('boom')

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.listen(backlog=1)


@uringio_test
async def test_socket_context_manager__original_exception_propagates():
    sock = await uringio.prep_socket(domain=socket.AF_INET)

    with pytest.raises(expected_exception=ValueError, match='specific error'):
        async with sock:
            raise ValueError('specific error')


@uringio_test
async def test_socket_context_manager__aenter_returns_self():
    sock = await uringio.prep_socket(domain=socket.AF_INET)

    result = await sock.__aenter__()
    assert result is sock

    await sock.close()


@uringio_test
async def test_socket_context_manager__usable_for_bind_listen():
    async with (await uringio.prep_socket(domain=socket.AF_INET)) as sock:
        await sock.bind(host='127.0.0.1', port=0)
        await sock.listen(backlog=1)

    with pytest.raises(expected_exception=BrokenPipeError):
        sock.listen(backlog=1)
