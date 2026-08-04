import socket
import sys

sys.path.insert(0, '')

import puring
import pytest

from tests.python_tests.tests_utils.pytest_param import pytest_param, pytest_parametrize
from tests.python_tests.tests_utils.runner import puring_test


@pytest_parametrize(
    ('domain'),
    (
        pytest_param(domain='not_an_int', id='domain_wrong_type_str'),
        pytest_param(domain=1.5, id='domain_wrong_type_float'),
    ),
)
@puring_test
async def test_prep_socket__validation_error(domain):
    with pytest.raises(expected_exception=TypeError):
        puring.prep_socket(domain=domain)


@puring_test
async def test_prep_socket__default_domain_success():
    sock = await puring.prep_socket()
    assert sock is not None

    await sock.close()


@pytest_parametrize(
    ('domain'),
    (
        pytest_param(domain=socket.AF_INET, id='af_inet'),
        pytest_param(domain=socket.AF_INET6, id='af_inet6'),
    ),
)
@puring_test
async def test_prep_socket__explicit_domain_success(domain):
    sock = await puring.prep_socket(domain=domain)
    assert sock is not None

    await sock.close()


@puring_test
async def test_prep_socket__no_await_does_not_crash():
    assert puring.prep_socket()
