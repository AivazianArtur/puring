import asyncio
import itertools
import os
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

import aio_uring


# ---- fd limit ------------------------------------------------------
# Each connection costs ~2 fds (client side + accepted server side).
# At the top CONN_COUNTS value that's already close to the default
# soft ulimit (1024 on many distros), and fds from an earlier backend
# in the same run may not have been reclaimed yet by the OS/GC when
# the next backend starts - so raise the soft limit up to the hard
# limit once at startup instead of silently failing mid-sweep with
# "Too many open files" partway through the CONN_COUNTS sweep.

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


# ~3 fds/connection in flight (client socket, accepted server socket,
# +1 slack for whatever's still being torn down from the previous
# step), plus a fixed reserve for stdio/listening sockets/interpreter
# internals. This is deliberately conservative - better to clip
# CONN_COUNTS down and finish the sweep than crash halfway through it.
FDS_PER_CONN = 3
FD_RESERVE = 64


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


# ---- Config -----------------------------------------------------------
# Here: N connections open *simultaneously* and each does
# ROUNDS_PER_CONN ping-pong exchanges of MESSAGE_SIZE bytes, all in
# flight at once. This is the socket equivalent of nvme_saturation_test's
# depth sweep - it's the shape of workload io_uring is actually built
# for: many independent I/O ops in flight, each with real work attached,
# not just "can you accept()".
#
# thread-per-connection is included and expected to visibly degrade as
# N grows - context-switch/scheduling cost per thread doesn't disappear
# just because the payload is real now.

CONN_COUNTS = [50, 200, 800]
ROUNDS_PER_CONN = 100
MESSAGE_SIZE = 4 * 1024  # 4 KiB per round-trip, deliberately small so
                         # per-op dispatch overhead is what's on trial,
                         # not raw bandwidth (see vectored_write.py for
                         # that story)
DATA = os.urandom(MESSAGE_SIZE)

HOST = '127.0.0.1'
_PORT_COUNTER = itertools.count(9500)


def next_port():
    # A fresh port per sweep step, per backend, so a listening socket
    # that the OS hasn't fully released yet from the previous step can
    # never collide with the next bind() - sidesteps the EADDRINUSE
    # entirely instead of relying on SO_REUSEADDR + timing.
    return next(_PORT_COUNTER)


def total_bytes(n):
    # round trip = send + recv, both directions carry MESSAGE_SIZE
    return n * ROUNDS_PER_CONN * MESSAGE_SIZE * 2


def mbps(n, seconds):
    return total_bytes(n) / seconds / (1024 ** 2)


def roundtrips_per_sec(n, seconds):
    return (n * ROUNDS_PER_CONN) / seconds


def recv_exact(sock_recv, n):
    buf = bytearray()
    while len(buf) < n:
        chunk = sock_recv(n - len(buf))
        if not chunk:
            raise ConnectionError('peer closed early')
        buf.extend(chunk)
    return bytes(buf)


# ---------------------------------------------------------------------
# 1. stdlib: thread-per-connection, both server-side handler and
#    client-side driver get their own OS thread. n threads doing real
#    ping-pong work concurrently, not just connecting.
# ---------------------------------------------------------------------

def _stdlib_handle(conn):
    try:
        for _ in range(ROUNDS_PER_CONN):
            data = recv_exact(conn.recv, MESSAGE_SIZE)
            conn.sendall(data)
    finally:
        conn.close()


def _stdlib_server(port, n, ready_event, stop_event):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, port))
    srv.listen(4096)
    srv.settimeout(0.2)
    ready_event.set()

    accepted = 0
    threads = []
    while accepted < n:
        try:
            conn, _ = srv.accept()
        except socket.timeout:
            if stop_event.is_set():
                break
            continue
        th = threading.Thread(target=_stdlib_handle, args=(conn,), daemon=True)
        th.start()
        threads.append(th)
        accepted += 1

    for th in threads:
        th.join()
    srv.close()


def _stdlib_client(port):
    c = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    c.connect((HOST, port))
    for _ in range(ROUNDS_PER_CONN):
        c.sendall(DATA)
        recv_exact(c.recv, MESSAGE_SIZE)
    c.close()


def stdlib_fanout(n, port):
    print(f'Running stdlib thread-per-connection fanout, n={n}')

    ready_event = threading.Event()
    stop_event = threading.Event()
    server_thread = threading.Thread(
        target=_stdlib_server, args=(port, n, ready_event, stop_event)
    )
    server_thread.start()
    ready_event.wait()

    start = time.perf_counter()
    with ThreadPoolExecutor(max_workers=n) as pool:
        list(pool.map(_stdlib_client, [port] * n))
    elapsed = time.perf_counter() - start

    stop_event.set()
    server_thread.join()
    return elapsed


# ---------------------------------------------------------------------
# 2/3. asyncio & uvloop streams: n connections as n pairs of tasks on a
#    single event loop, no per-connection OS thread.
# ---------------------------------------------------------------------

async def _asyncio_fanout(n, port):
    async def handle(reader, writer):
        try:
            for _ in range(ROUNDS_PER_CONN):
                data = await reader.readexactly(MESSAGE_SIZE)
                writer.write(data)
                await writer.drain()
        finally:
            writer.close()

    server = await asyncio.start_server(handle, HOST, port)
    async with server:
        async def one():
            reader, writer = await asyncio.open_connection(HOST, port)
            for _ in range(ROUNDS_PER_CONN):
                writer.write(DATA)
                await writer.drain()
                await reader.readexactly(MESSAGE_SIZE)
            writer.close()
            await writer.wait_closed()

        start = time.perf_counter()
        await asyncio.gather(*[one() for _ in range(n)])
        elapsed = time.perf_counter() - start

    return elapsed


async def asyncio_fanout(n, port):
    print(f'Running asyncio streams fanout, n={n}')
    return await _asyncio_fanout(n, port)


async def uvloop_fanout(n, port):
    print(f'Running uvloop streams fanout, n={n}')
    return await _asyncio_fanout(n, port)


# ---------------------------------------------------------------------
# 4. aio_uring: server + all n clients on the single io_uring ring, every
#    send/recv is its own SQE, no worker threads, no epoll.
# ---------------------------------------------------------------------

async def aio_uring_fanout(n, port):
    print(f'Running aio_uring sockets fanout (io_uring), n={n}')

    server_sock = await aio_uring.prep_socket(
        domain=socket.AF_INET, socktype=socket.SOCK_STREAM
    )
    await server_sock.bind(host=HOST, port=port)
    await server_sock.listen(backlog=4096)

    async def handle(conn):
        try:
            for _ in range(ROUNDS_PER_CONN):
                received = 0
                buf = b''
                while received < MESSAGE_SIZE:
                    chunk = await conn.recv(bufsize=MESSAGE_SIZE - received)
                    if not chunk:
                        raise ConnectionError('peer closed early')
                    buf += chunk
                    received += len(chunk)
                await conn.send(data=buf)
        finally:
            await conn.close()

    async def accept_loop(count):
        for _ in range(count):
            conn = await server_sock.accept()
            asyncio.create_task(handle(conn))

    async def one():
        c = await aio_uring.prep_socket(
            domain=socket.AF_INET, socktype=socket.SOCK_STREAM
        )
        await c.connect(host=HOST, port=port)
        for _ in range(ROUNDS_PER_CONN):
            await c.send(data=DATA)
            received = 0
            while received < MESSAGE_SIZE:
                chunk = await c.recv(bufsize=MESSAGE_SIZE - received)
                if not chunk:
                    raise ConnectionError('peer closed early')
                received += len(chunk)
        await c.close()

    accept_task = asyncio.create_task(accept_loop(n))

    start = time.perf_counter()
    await asyncio.gather(*[one() for _ in range(n)])
    elapsed = time.perf_counter() - start

    await accept_task
    await server_sock.close()
    return elapsed


def run():
    print(f'{CONN_COUNTS=}, {ROUNDS_PER_CONN=}, {MESSAGE_SIZE=}')

    # ~3 fds/connection at peak, +64 reserve for stdio/interpreter/
    # listening sockets. Ask for headroom, then clip the sweep to
    # whatever we actually got - never crash mid-sweep over this.
    needed = max(CONN_COUNTS) * FDS_PER_CONN + FD_RESERVE
    soft, hard = raise_fd_limit(needed)
    conn_counts = clip_conn_counts(CONN_COUNTS, soft)

    results = []
    skipped = []

    def run_step(label, fn, n, *args):
        # A step failing (fd exhaustion, a transient bind race, etc.)
        # should cost you that one data point, not the rest of the
        # sweep - especially with 4+ backends x 3 depths, losing an
        # hour of runtime to one flaky step is worse than a gap in
        # the table.
        try:
            t = fn(n, *args)
            results.append((label, n, t))
        except OSError as e:
            print(f'  SKIPPED {label} n={n}: {e!r}')
            skipped.append((label, n, e))
        time.sleep(0.3)  # let the OS reclaim fds/ports before the next step

    for n in conn_counts:
        run_step('stdlib thread/conn', stdlib_fanout, n, next_port())

    for n in conn_counts:
        run_step('asyncio', lambda n, p: asyncio.run(asyncio_fanout(n, p)), n, next_port())

    if is_uvloop_installed:
        uvloop.install()
        for n in conn_counts:
            run_step('uvloop', lambda n, p: asyncio.run(uvloop_fanout(n, p)), n, next_port())

    with asyncio.Runner(loop_factory=aio_uring.AioUringLoop) as runner:
        for n in conn_counts:
            run_step('aio_uring', lambda n, p: runner.run(aio_uring_fanout(n, p)), n, next_port())

    print('\n==== RESULTS ====')
    print(f'{"backend":20s} {"n":>6s} {"MB/s":>10s} {"round-trips/s":>15s} {"time":>8s}')
    for name, n, sec in results:
        print(
            f'{name:20s} {n:6d} '
            f'{mbps(n, sec):10.2f} '
            f'{roundtrips_per_sec(n, sec):15.0f} '
            f'{sec:7.3f}s'
        )

    if skipped:
        print('\n==== SKIPPED ====')
        for label, n, e in skipped:
            print(f'{label} n={n}: {e!r}')


if __name__ == '__main__':
    run()
