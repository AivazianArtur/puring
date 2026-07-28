import asyncio
from unittest.mock import AsyncMock, Mock

import pytest

@pytest.fixture(scope='session')
def app():
    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        yield runner
