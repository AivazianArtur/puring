import sys

sys.path.insert(0, '')

import asyncio
import uringio
from functools import wraps


def uringio_test(fn):
    @wraps(fn)
    def wrapper(*args, **kwargs):
        with asyncio.Runner(loop_factory=uringio.UringioLoop) as runner:
            runner.run(fn(*args, **kwargs))
    return wrapper
