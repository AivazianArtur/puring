import subprocess, sys

from config import CONNECTIONS, MESSAGES, MSG_SIZE

# WARNING: Run server_asyncio.py first

folder = 'docs/benchmark/sockets/'
tests = [
    folder+'client_asyncio.py',
    folder+'client_uvloop.py',
    folder+'client_puring.py',
]

print(f'{CONNECTIONS = } \n {MESSAGES = } \n {MSG_SIZE= }')
for t in tests:
    print('\n==============================')
    subprocess.run([sys.executable, t])
