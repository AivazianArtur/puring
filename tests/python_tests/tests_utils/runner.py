import sys

sys.path.insert(0, '')

import asyncio
import aio_uring
from functools import wraps


def aio_uring_test(fn):
    @wraps(fn)
    def wrapper(*args, **kwargs):
        with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
            runner.run(fn(*args, **kwargs))
    return wrapper
