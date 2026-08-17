import os
import subprocess
from setuptools import Extension, setup
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SRC_DIR = ROOT / 'src'

LIBURING_DIR = ROOT / 'vendor' / 'liburing'
LIBURING_LIB = LIBURING_DIR / 'src' / 'liburing.a'


def build_liburing() -> None:
    env = os.environ.copy()
    env.pop('CFLAGS', None)
    env.pop('LDFLAGS', None)
    subprocess.run(['./configure'], cwd=str(LIBURING_DIR), check=True, env=env)
    subprocess.run(
        ['make', '-C', str(LIBURING_DIR), 'library'],
        check=True,
        env=env,
    )

build_liburing()


def rel(path: Path) -> str:
    return os.path.relpath(str(path), str(ROOT))


sources = [rel(path) for path in SRC_DIR.rglob('*.c')]
include_dirs = sorted({
    rel(path.parent) for path in SRC_DIR.rglob('*.h')
})
include_dirs += [rel(LIBURING_DIR / 'src' / 'include')]

extra_compile_args = []
if os.environ.get('AIO_URING_DEBUG'):
    extra_compile_args.append('-DAIO_URING_DEBUG')


ext = Extension(
    'aio_uring',
    sources=sources,
    include_dirs=include_dirs,
    extra_objects=[rel(LIBURING_LIB)],
    extra_compile_args=extra_compile_args,
)

setup(ext_modules=[ext])
