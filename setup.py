import os
from setuptools import setup, Extension
from pathlib import Path

sources = [str(p) for p in Path('src').rglob('*.c')]
headers = sorted({str(p.parent) for p in Path('src').rglob('*.h')})
headers = headers + ['requirements/liburing/src/include']

ldflags = os.environ.get('LDFLAGS', "").split()

extra_compile_args = []
if os.environ.get('PURING_DEBUG'):
    extra_compile_args.append('-DPURING_DEBUG')


ext = Extension(
    'puring',
    sources=sources,
    include_dirs=headers,
    extra_objects=['requirements/liburing/src/liburing.a'],
    extra_link_args=ldflags,
    extra_compile_args=extra_compile_args,
)

setup(ext_modules=[ext])
