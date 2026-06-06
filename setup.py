from setuptools import setup, Extension
from pathlib import Path

sources = [str(p) for p in Path('src').rglob('*.c')]
headers = sorted({str(p.parent) for p in Path('src').rglob('*.h')})
include_dirs = headers + ['requirements/liburing/src/include']

ext = Extension(
    'puring',
    sources=sources,
    include_dirs=include_dirs,
    extra_objects=['requirements/liburing/src/liburing.a'],
)

setup(ext_modules=[ext])
