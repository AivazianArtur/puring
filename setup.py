import os
from setuptools import setup, Extension
from pathlib import Path


liburing_dir = os.path.join('requirements', 'liburing', 'src')
liburing_include = os.path.join(liburing_dir, 'include')
liburing_lib = liburing_dir

src = Path('src')
sources = [str(p) for p in src.rglob('*.c')]

ext = Extension(
    'puring',
    sources=sources,
    include_dirs=[
        'src',
        'src/core',
        'src/ops',
        'src/python_api',
        'requirements/liburing',
        'requirements/liburing/src',
        'requirements/liburing/include',
        liburing_include,
    ],
    extra_objects=[
        'requirements/liburing/src/liburing.a',
    ],
    library_dirs=[
        'requirements/liburing/src',
    ],
    # libraries=['uring'],
    # extra_link_args=[
    #     '-Wl,-rpath,$ORIGIN/../requirements/liburing/src'
    # ],
)

setup(
    name='puring',
    version='0.2.0',
    ext_modules=[ext],
)
