import asyncio
import socket
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor

is_uvloop_installed = False
try:
    import uvloop
    is_uvloop_installed = True
except ImportError:
    pass

sys.path.insert(0, '')

import puring

# NOTE ON THE PURING SOCKET API: prep_socket/bind/listen/accept/connect/
# send/recv/close are reconstructed from sockets.c. `accept()` is assumed
# to resolve to a new connected PuringSocket, mirroring how open_file()
# hands back a PuringFile. Adjust if your build's names differ.


# ---- Config -----------------------------------------------------------
# This isn't a throughput test - each connection moves 4 bytes each way.
# The question is purely "how does the cost of N *concurrent* connections
# scale", which is where thread-per-connection historically falls over
# (the actual C10K problem) and where event-loop/io_uring designs are
# supposed to shine.
#
# Raise CONN_COUNTS if your ulimit -n allows it; each connection needs
# ~2 file descriptors (client + accepted server side) plus whatever the
# backend needs for bookkeeping.

CONN_COUNTS = [100, 500, 2000]

MESSAGE = b'ping'
REPLY = b'pong'

HOST = '127.0.0.1'
PORT_STDLIB = 9401
PORT_ASYNCIO = 9402
PORT_UVLOOP = 9403
PORT_PURING = 9404


def conns_per_sec(n, seconds):
    return n / seconds


# ---------------------------------------------------------------------
# 1. stdlib: thread-per-connection, both server and client side. This is
#    the classic pre-C10K-fix design - included specifically because it
#    should visibly stop scaling as n grows.
# ---------------------------------------------------------------------

def _stdlib_server(port, ready_event, stop_event):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, port))
    srv.listen(4096)
    srv.settimeout(0.2)
    ready_event.set()

    def handle(conn):
        try:
            conn.recv(len(MESSAGE))
            conn.sendall(REPLY)
        finally:
            conn.close()

    while not stop_event.is_set():
        try:
            conn, _ = srv.accept()
        except socket.timeout:
            continue
        threading.Thread(target=handle, args=(conn,), daemon=True).start()

    srv.close()


def stdlib_c10k(n):
    print(f'Running stdlib thread-per-connection, n={n}')

    ready_event = threading.Event()
    stop_event = threading.Event()
    server_thread = threading.Thread(
        target=_stdlib_server, args=(PORT_STDLIB, ready_event, stop_event)
    )
    server_thread.start()
    ready_event.wait()

    def one(_):
        c = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        c.connect((HOST, PORT_STDLIB))
        c.sendall(MESSAGE)
        c.recv(len(REPLY))
        c.close()

    start = time.perf_counter()
    with ThreadPoolExecutor(max_workers=n) as pool:
        list(pool.map(one, range(n)))
    elapsed = time.perf_counter() - start

    stop_event.set()
    server_thread.join()
    return elapsed


# ---------------------------------------------------------------------
# 2/3. asyncio & uvloop streams: server and all n clients live as tasks
#    on a single event loop, no per-connection OS thread.
# ---------------------------------------------------------------------

async def _asyncio_c10k(n, port):
    async def handle(reader, writer):
        try:
            await reader.readexactly(len(MESSAGE))
            writer.write(REPLY)
            await writer.drain()
        finally:
            writer.close()

    server = await asyncio.start_server(handle, HOST, port)
    async with server:
        async def one():
            reader, writer = await asyncio.open_connection(HOST, port)
            writer.write(MESSAGE)
            await writer.drain()
            await reader.readexactly(len(REPLY))
            writer.close()
            await writer.wait_closed()

        start = time.perf_counter()
        await asyncio.gather(*[one() for _ in range(n)])
        elapsed = time.perf_counter() - start

    return elapsed


async def asyncio_c10k(n):
    print(f'Running asyncio streams, n={n}')
    return await _asyncio_c10k(n, PORT_ASYNCIO)


async def uvloop_c10k(n):
    print(f'Running uvloop streams, n={n}')
    return await _asyncio_c10k(n, PORT_UVLOOP)


# ---------------------------------------------------------------------
# 4. puring: server + all n clients on the single io_uring ring, no
#    worker threads and no epoll either.
# ---------------------------------------------------------------------

async def puring_c10k(n):
    print(f'Running puring sockets (io_uring), n={n}')

    server_sock = await puring.prep_socket(
        domain=socket.AF_INET, socktype=socket.SOCK_STREAM
    )
    await server_sock.bind(host=HOST, port=PORT_PURING)
    await server_sock.listen(backlog=4096)

    async def handle(conn):
        try:
            await conn.recv(bufsize=len(MESSAGE))
            await conn.send(data=REPLY)
        finally:
            await conn.close()

    async def accept_loop(count):
        for _ in range(count):
            conn = await server_sock.accept()
            asyncio.create_task(handle(conn))

    async def one():
        c = await puring.prep_socket(
            domain=socket.AF_INET, socktype=socket.SOCK_STREAM
        )
        await c.connect(host=HOST, port=PORT_PURING)
        await c.send(data=MESSAGE)
        await c.recv(bufsize=len(REPLY))
        await c.close()

    accept_task = asyncio.create_task(accept_loop(n))

    start = time.perf_counter()
    await asyncio.gather(*[one() for _ in range(n)])
    elapsed = time.perf_counter() - start

    await accept_task
    await server_sock.close()
    return elapsed


def run():
    print(f'{CONN_COUNTS=}')
    results = []

    for n in CONN_COUNTS:
        t = stdlib_c10k(n)
        results.append(('stdlib thread/conn', n, t))

    for n in CONN_COUNTS:
        t = asyncio.run(asyncio_c10k(n))
        results.append(('asyncio', n, t))

    if is_uvloop_installed:
        uvloop.install()
        for n in CONN_COUNTS:
            t = asyncio.run(uvloop_c10k(n))
            results.append(('uvloop', n, t))

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        for n in CONN_COUNTS:
            t = runner.run(puring_c10k(n))
            results.append(('puring', n, t))

    print('\n==== RESULTS ====')
    print(f'{"backend":20s} {"n":>6s} {"conns/s":>10s} {"time":>8s}')
    for name, n, sec in results:
        print(f'{name:20s} {n:6d} {conns_per_sec(n, sec):10.0f} {sec:7.3f}s')


if __name__ == '__main__':
    run()
