import sys

sys.path.insert(0, '')

import uringio
import pytest

from tests.python_tests.tests_utils.runner import uringio_test


@uringio_test
async def test_close__success():
    sock = await uringio.prep_socket()

    result = await sock.close()
    assert result == 0


@uringio_test
async def test_close__double_close_raises_error():
    sock = await uringio.prep_socket()
    await sock.close()

    with pytest.raises(expected_exception=OSError):
        await sock.close()


@uringio_test
async def test_close__after_bind():
    sock = await uringio.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)

    result = await sock.close()
    assert result == 0


@uringio_test
async def test_close__after_bind_and_listen():
    sock = await uringio.prep_socket()
    await sock.bind(host='127.0.0.1', port=0)
    await sock.listen(backlog=1)

    result = await sock.close()
    assert result == 0
