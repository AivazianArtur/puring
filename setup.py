from setuptools import setup, Extension
from pathlib import Path
import os

sources = [str(p) for p in Path('src').rglob('*.c')]
headers = sorted({str(p.parent) for p in Path('src').rglob('*.h')})

ldflags = os.environ.get('LDFLAGS', "").split()

ext = Extension(
    'puring',
    sources=sources,
    include_dirs=headers + ['requirements/liburing/src/include'],
    extra_objects=['requirements/liburing/src/liburing.a'],
    extra_link_args=ldflags,
)

setup(ext_modules=[ext])
