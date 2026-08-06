import asyncio
import os
import socket
import sys
import threading
import time

is_uvloop_installed = False
try:
    import uvloop
    is_uvloop_installed = True
except ImportError:
    pass

sys.path.insert(0, '')

import puring

# NOTE ON THE PURING SOCKET API: the exact method names/kwargs below
# (prep_socket, bind, connect, send, recv, close) are reconstructed from
# sockets.c. Adjust if your build's Python-facing names differ.


# ---- Config -----------------------------------------------------------
# Single connection, ping-pong echo: client sends a chunk, waits for the
# full chunk to come back before sending the next one. This is a
# request/response (latency-bound) pattern, not a one-way stream, so it
# also gives you round-trips/sec alongside raw MB/s.

HOST = '127.0.0.1'
MESSAGE_SIZE = 64 * 1024
ITERATIONS = 2000
TOTAL = MESSAGE_SIZE * ITERATIONS
DATA = os.urandom(MESSAGE_SIZE)

PORT_STDLIB = 9201
PORT_ASYNCIO = 9202
PORT_UVLOOP = 9203
PORT_PURING = 9204


def mbps(seconds):
    return TOTAL / seconds / (1024 ** 2)


def rtps(seconds):
    return ITERATIONS / seconds


def recv_exact(sock_recv, n):
    """Drain exactly n bytes via repeated sock.recv() calls."""
    buf = bytearray()
    while len(buf) < n:
        chunk = sock_recv(n - len(buf))
        if not chunk:
            raise ConnectionError('peer closed early')
        buf.extend(chunk)
    return bytes(buf)


# ---------------------------------------------------------------------
# 1. stdlib blocking sockets: echo server runs in a background thread.
# ---------------------------------------------------------------------

def stdlib_echo():
    print('Running stdlib blocking sockets (thread server)')

    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((HOST, PORT_STDLIB))
    server_sock.listen(1)

    def server():
        conn, _ = server_sock.accept()
        try:
            for _ in range(ITERATIONS):
                data = recv_exact(lambda n: conn.recv(n), MESSAGE_SIZE)
                conn.sendall(data)
        finally:
            conn.close()

    t = threading.Thread(target=server)
    t.start()

    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((HOST, PORT_STDLIB))

    start = time.perf_counter()
    for _ in range(ITERATIONS):
        client.sendall(DATA)
        recv_exact(lambda n: client.recv(n), MESSAGE_SIZE)
    elapsed = time.perf_counter() - start

    client.close()
    t.join()
    server_sock.close()
    return elapsed


# ---------------------------------------------------------------------
# 2. asyncio streams: the idiomatic asyncio.start_server / open_connection
#    echo, single connection, same ping-pong pattern.
# ---------------------------------------------------------------------

async def asyncio_echo():
    print('Running asyncio streams echo')

    async def handle(reader, writer):
        try:
            for _ in range(ITERATIONS):
                data = await reader.readexactly(MESSAGE_SIZE)
                writer.write(data)
                await writer.drain()
        finally:
            writer.close()

    server = await asyncio.start_server(handle, HOST, PORT_ASYNCIO)
    async with server:
        reader, writer = await asyncio.open_connection(HOST, PORT_ASYNCIO)

        start = time.perf_counter()
        for _ in range(ITERATIONS):
            writer.write(DATA)
            await writer.drain()
            await reader.readexactly(MESSAGE_SIZE)
        elapsed = time.perf_counter() - start

        writer.close()
        await writer.wait_closed()

    return elapsed


async def uvloop_echo():
    print('Running uvloop streams echo')

    async def handle(reader, writer):
        try:
            for _ in range(ITERATIONS):
                data = await reader.readexactly(MESSAGE_SIZE)
                writer.write(data)
                await writer.drain()
        finally:
            writer.close()

    server = await asyncio.start_server(handle, HOST, PORT_UVLOOP)
    async with server:
        reader, writer = await asyncio.open_connection(HOST, PORT_UVLOOP)

        start = time.perf_counter()
        for _ in range(ITERATIONS):
            writer.write(DATA)
            await writer.drain()
            await reader.readexactly(MESSAGE_SIZE)
        elapsed = time.perf_counter() - start

        writer.close()
        await writer.wait_closed()

    return elapsed


# ---------------------------------------------------------------------
# 3. puring: server + client both as io_uring sockets on the same ring,
#    no worker threads, no OS-level select/epoll either.
# ---------------------------------------------------------------------

async def puring_echo():
    print('Running puring sockets echo (io_uring)')

    server_sock = await puring.prep_socket(
        domain=socket.AF_INET, socktype=socket.SOCK_STREAM
    )
    await server_sock.bind(host=HOST, port=PORT_PURING)
    await server_sock.listen(backlog=16)

    async def server():
        # assumption: accept() resolves to a new connected PuringSocket,
        # mirroring how open_file() hands back a PuringFile.
        conn = await server_sock.accept()
        try:
            for _ in range(ITERATIONS):
                received = 0
                while received < MESSAGE_SIZE:
                    chunk = await conn.recv(bufsize=MESSAGE_SIZE - received)
                    if not chunk:
                        raise ConnectionError('peer closed early')
                    await conn.send(data=chunk)
                    received += len(chunk)
        finally:
            await conn.close()

    server_task = asyncio.create_task(server())

    client = await puring.prep_socket(
        domain=socket.AF_INET, socktype=socket.SOCK_STREAM
    )
    await client.connect(host=HOST, port=PORT_PURING)

    start = time.perf_counter()
    for _ in range(ITERATIONS):
        await client.send(data=DATA)
        received = 0
        while received < MESSAGE_SIZE:
            chunk = await client.recv(bufsize=MESSAGE_SIZE - received)
            if not chunk:
                raise ConnectionError('peer closed early')
            received += len(chunk)
    elapsed = time.perf_counter() - start

    await client.close()
    await server_task
    await server_sock.close()

    return elapsed


def run():
    print(f'{MESSAGE_SIZE=}, {ITERATIONS=}, total one-way={TOTAL / (1024 ** 2):.0f} MiB')

    results = []

    t = stdlib_echo()
    results.append(('stdlib blocking', t))

    t = asyncio.run(asyncio_echo())
    results.append(('asyncio streams', t))

    if is_uvloop_installed:
        uvloop.install()
        t = asyncio.run(uvloop_echo())
        results.append(('uvloop streams', t))

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        t = runner.run(puring_echo())
        results.append(('puring sockets', t))

    print('\n==== RESULTS ====')
    print(f'{"backend":18s} {"MB/s":>10s} {"round-trips/s":>15s} {"time":>8s}')
    for name, sec in results:
        print(f'{name:18s} {mbps(sec):10.2f} {rtps(sec):15.0f} {sec:7.3f}s')


if __name__ == '__main__':
    run()
