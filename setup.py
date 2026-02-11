from setuptools import setup, Extension
from pathlib import Path

src = Path('src')

sources = [str(p) for p in src.rglob('*.c')]

ext = Extension(
    name='loop',
    sources=sources,
    include_dirs=[
        'src',
        'src/core',
        'src/ops',
        'src/python_api',
    ],
    libraries=['uring'],
    extra_compile_args=['-O2', '-Wall', '-Wextra'],
)

setup(
    name='puring',
    version='0.1.0',
    ext_modules=[ext],
)
