import subprocess, sys

folder = 'docs/benchmark/sockets/'
tests = [
    folder+'client_asyncio.py',
    folder+'client_uvloop.py',
    folder+'client_puring.py',
]


for t in tests:
    print('\n==============================')
    subprocess.run([sys.executable, t])
