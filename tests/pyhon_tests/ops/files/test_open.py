import os
import sys

sys.path.insert(0, '')

import asyncio
import puring

from unittest import Mock
from tests.pyhon_tests.utils.pytest_param import pytest_param, pytest_parametrize


@pytest_parametrize(
    (
        'a',
        'b',
        'c',
        'd',
    ),
    (
        pytest_param(
            a=...,
            b=...,
            c=...,
            d=...,
            id='...',
        ),
    ),
)
async def test_open__success(
    controller_result: int,
    is_event_sent: bool,
    expected_status_code: int,
    expected_event_context: int | None,
):
    tasks = Mock()
    tasks.add_task = Mock()


@pytest_parametrize(
    (
        'a',
        'b',
        'c',
        'd',
    ),
    (
        pytest_param(
            a=...,
            b=...,
            c=...,
            d=...,
            id='...',
        ),
    ),
)
async def test_open__fail(
    controller_result: int,
    is_event_sent: bool,
    expected_status_code: int,
    expected_event_context: int | None,
):
    tasks = Mock()
    tasks.add_task = Mock()
