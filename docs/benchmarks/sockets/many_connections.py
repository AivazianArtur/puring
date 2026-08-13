import asyncio
import itertools
import resource
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

# ---- Config -----------------------------------------------------------
# This isn't a throughput test - each connection moves 4 bytes each way.
# The question is purely "how does the cost of N *concurrent* connections
# scale", which is where thread-per-connection historically falls over
# (the actual C10K problem) and where event-loop/io_uring designs are
# supposed to shine.
#
# Raise CONN_COUNTS if your ulimit -n allows it; each connection needs
# ~2 file descriptors (client + accepted server side) plus whatever the
# backend needs for bookkeeping. See fd-limit handling below - the
# sweep clips itself to what's actually available instead of crashing
# partway through.

CONN_COUNTS = [100, 500, 2000]

MESSAGE = b'ping'
REPLY = b'pong'

HOST = '127.0.0.1'
_PORT_COUNTER = itertools.count(9400)


def next_port():
    # A fresh port per sweep step, per backend, so a listening socket
    # the OS hasn't fully released yet from a previous step can never
    # collide with the next bind() - this is what caused EADDRINUSE
    # when PORT_PURING was reused across n=100/500/2000.
    return next(_PORT_COUNTER)


# ---- fd limit ------------------------------------------------------
# Each connection costs ~2 fds (client + accepted server side) at
# peak, plus whatever's still being torn down from the previous step.
# Raise the soft limit to the hard limit once at startup, then clip
# the sweep to whatever's actually available - never crash mid-sweep
# over this (same approach as concurrent_echo_fanout.py).

FDS_PER_CONN = 3
FD_RESERVE = 64


def raise_fd_limit(min_needed):
    soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
    target = min(max(min_needed, soft), hard)
    if target > soft:
        try:
            resource.setrlimit(resource.RLIMIT_NOFILE, (target, hard))
        except (ValueError, OSError) as e:
            print(
                f'warning: could not raise RLIMIT_NOFILE to {target} '
                f'(soft={soft}, hard={hard}): {e}. '
                f'Run `ulimit -n {min_needed}` before starting Python, '
                f'or lower CONN_COUNTS below.'
            )
    return resource.getrlimit(resource.RLIMIT_NOFILE)


def clip_conn_counts(counts, available_fds):
    max_n = max(1, (available_fds - FD_RESERVE) // FDS_PER_CONN)
    clipped = sorted({min(n, max_n) for n in counts})
    if clipped != sorted(set(counts)):
        print(
            f'warning: clipping CONN_COUNTS to fit {available_fds} available fds '
            f'(~{FDS_PER_CONN} fds/conn + {FD_RESERVE} reserve -> max n={max_n}). '
            f'Requested {counts}, using {clipped}. '
            f'Raise `ulimit -n` and rerun for the original values.'
        )
    return clipped


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


def stdlib_c10k(n, port):
    print(f'Running stdlib thread-per-connection, n={n}')

    ready_event = threading.Event()
    stop_event = threading.Event()
    server_thread = threading.Thread(
        target=_stdlib_server, args=(port, ready_event, stop_event)
    )
    server_thread.start()
    ready_event.wait()

    def one(_):
        c = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        c.connect((HOST, port))
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


async def asyncio_c10k(n, port):
    print(f'Running asyncio streams, n={n}')
    return await _asyncio_c10k(n, port)


async def uvloop_c10k(n, port):
    print(f'Running uvloop streams, n={n}')
    return await _asyncio_c10k(n, port)


# ---------------------------------------------------------------------
# 4. puring: server + all n clients on the single io_uring ring, no
#    worker threads and no epoll either.
# ---------------------------------------------------------------------

async def puring_c10k(n, port):
    print(f'Running puring sockets (io_uring), n={n}')

    server_sock = await puring.prep_socket(
        domain=socket.AF_INET, socktype=socket.SOCK_STREAM
    )
    await server_sock.bind(host=HOST, port=port)
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
        await c.connect(host=HOST, port=port)
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

    # ~3 fds/connection at peak, +64 reserve for stdio/interpreter/
    # listening sockets. Ask for headroom, then clip the sweep to
    # whatever we actually got - never crash mid-sweep over this.
    needed = max(CONN_COUNTS) * FDS_PER_CONN + FD_RESERVE
    soft, hard = raise_fd_limit(needed)
    conn_counts = clip_conn_counts(CONN_COUNTS, soft)

    results = []
    skipped = []

    def run_step(label, fn, n, *args):
        try:
            t = fn(n, *args)
            results.append((label, n, t))
        except OSError as e:
            print(f'  SKIPPED {label} n={n}: {e!r}')
            skipped.append((label, n, e))
        time.sleep(0.3)  # let the OS reclaim fds/ports before the next step

    for n in conn_counts:
        run_step('stdlib thread/conn', stdlib_c10k, n, next_port())

    for n in conn_counts:
        run_step('asyncio', lambda n, p: asyncio.run(asyncio_c10k(n, p)), n, next_port())

    if is_uvloop_installed:
        uvloop.install()
        for n in conn_counts:
            run_step('uvloop', lambda n, p: asyncio.run(uvloop_c10k(n, p)), n, next_port())

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        for n in conn_counts:
            run_step('puring', lambda n, p: runner.run(puring_c10k(n, p)), n, next_port())

    print('\n==== RESULTS ====')
    print(f'{"backend":20s} {"n":>6s} {"conns/s":>10s} {"time":>8s}')
    for name, n, sec in results:
        print(f'{name:20s} {n:6d} {conns_per_sec(n, sec):10.0f} {sec:7.3f}s')

    if skipped:
        print('\n==== SKIPPED ====')
        for label, n, e in skipped:
            print(f'{label} n={n}: {e!r}')


if __name__ == '__main__':
    run()
