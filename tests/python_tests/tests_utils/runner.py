import sys

sys.path.insert(0, '')

import asyncio
import puring
from functools import wraps


def puring_test(fn):
    @wraps(fn)
    def wrapper(*args, **kwargs):
        with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
            runner.run(fn(*args, **kwargs))
    return wrapper
