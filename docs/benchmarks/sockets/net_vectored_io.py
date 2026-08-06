import asyncio
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

# NOTE ON THE PURING SOCKET API: sendmsg()/recvmsg() kwargs (buffers,
# host, port, domain, is_poll_first) are reconstructed from sockets.c.
# Adjust if your build's Python-facing names differ.


# ---- Config -----------------------------------------------------------
# UDP, two "connected" sockets (connect() just fixes the default peer -
# valid for datagram sockets too, it lets us use plain send/recv without
# passing an address every call). Per iteration we move NR_CHUNKS
# buffers of CHUNK_SIZE bytes, then bounce them straight back (echo), so
# every backend gets a fair round-trip measurement.
#
# "scalar"   variant: NR_CHUNKS separate datagrams, one syscall/SQE each.
# "vectored" variant: 1 datagram assembled from NR_CHUNKS buffers via
#                      sendmsg (gather) / recvmsg (scatter).
#
# Keep NR_CHUNKS * CHUNK_SIZE comfortably under ~64 KiB - that's the
# practical UDP datagram ceiling, even on loopback.

CHUNK_SIZE = 4 * 1024
NR_CHUNKS = 8
ITERATIONS = 2000

TOTAL = CHUNK_SIZE * NR_CHUNKS * ITERATIONS
DATA_CHUNKS = [bytes([i % 256]) * CHUNK_SIZE for i in range(NR_CHUNKS)]

HOST = '127.0.0.1'

# distinct port pairs per backend/mode so runs never collide
PORTS = {
    'stdlib_scalar':    (9301, 9311),
    'stdlib_vectored':  (9302, 9312),
    'asyncio_vectored': (9303, 9313),
    'uvloop_vectored':  (9304, 9314),
    'puring_scalar':    (9305, 9315),
    'puring_vectored':  (9306, 9316),
}


def mbps(seconds):
    return TOTAL / seconds / (1024 ** 2)


def make_udp_pair(server_port, client_port):
    server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server.bind((HOST, server_port))

    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client.bind((HOST, client_port))

    server.connect((HOST, client_port))
    client.connect((HOST, server_port))
    return server, client


# ---------------------------------------------------------------------
# 1. stdlib scalar: NR_CHUNKS separate send()/recv() calls per iteration.
# ---------------------------------------------------------------------

def _scalar_server_loop(server):
    for _ in range(ITERATIONS):
        for _ in range(NR_CHUNKS):
            data = server.recv(CHUNK_SIZE)
            server.send(data)


def _scalar_client_loop(client):
    for _ in range(ITERATIONS):
        for chunk in DATA_CHUNKS:
            client.send(chunk)
        for _ in range(NR_CHUNKS):
            client.recv(CHUNK_SIZE)


def sync_scalar_udp():
    print('Running stdlib scalar UDP send/recv (NR_CHUNKS datagrams/iteration)')
    server_port, client_port = PORTS['stdlib_scalar']
    server, client = make_udp_pair(server_port, client_port)

    t = threading.Thread(target=_scalar_server_loop, args=(server,))
    t.start()

    start = time.perf_counter()
    _scalar_client_loop(client)
    elapsed = time.perf_counter() - start

    t.join()
    server.close()
    client.close()
    return elapsed


# ---------------------------------------------------------------------
# 2. stdlib vectored: 1 sendmsg()/recvmsg_into() call per iteration,
#    gathering/scattering all NR_CHUNKS buffers in one syscall.
# ---------------------------------------------------------------------

def _make_bufs():
    return [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]


def _vectored_server_loop(server):
    bufs = _make_bufs()
    for _ in range(ITERATIONS):
        server.recvmsg_into(bufs)
        server.sendmsg(bufs)


def _vectored_client_loop(client):
    bufs = _make_bufs()
    for _ in range(ITERATIONS):
        client.sendmsg(DATA_CHUNKS)
        client.recvmsg_into(bufs)


def sync_vectored_udp():
    print('Running stdlib vectored UDP sendmsg/recvmsg_into (1 datagram/iteration)')
    server_port, client_port = PORTS['stdlib_vectored']
    server, client = make_udp_pair(server_port, client_port)

    t = threading.Thread(target=_vectored_server_loop, args=(server,))
    t.start()

    start = time.perf_counter()
    _vectored_client_loop(client)
    elapsed = time.perf_counter() - start

    t.join()
    server.close()
    client.close()
    return elapsed


# ---------------------------------------------------------------------
# 3/4. asyncio & uvloop: no native vectored UDP transport in
#    asyncio.DatagramProtocol, so both wrap the same blocking sendmsg
#    calls via to_thread - this isolates event-loop/scheduling overhead
#    from the syscall itself.
# ---------------------------------------------------------------------

async def _threaded_vectored_udp(server_port, client_port):
    server, client = make_udp_pair(server_port, client_port)

    server_fut = asyncio.ensure_future(
        asyncio.to_thread(_vectored_server_loop, server)
    )

    start = time.perf_counter()
    await asyncio.to_thread(_vectored_client_loop, client)
    elapsed = time.perf_counter() - start

    await server_fut
    server.close()
    client.close()
    return elapsed


async def asyncio_vectored_udp():
    print('Running asyncio threadpool vectored UDP')
    return await _threaded_vectored_udp(*PORTS['asyncio_vectored'])


async def uvloop_vectored_udp():
    print('Running uvloop threadpool vectored UDP')
    return await _threaded_vectored_udp(*PORTS['uvloop_vectored'])


# ---------------------------------------------------------------------
# 5. puring scalar: NR_CHUNKS separate send()/recv() SQEs per iteration.
# ---------------------------------------------------------------------

async def puring_scalar_udp():
    print('Running puring scalar UDP send/recv (io_uring)')
    server_port, client_port = PORTS['puring_scalar']

    server = await puring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)
    await server.bind(host=HOST, port=server_port)
    await server.connect(host=HOST, port=client_port)

    client = await puring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)
    await client.bind(host=HOST, port=client_port)
    await client.connect(host=HOST, port=server_port)

    async def server_loop():
        for _ in range(ITERATIONS):
            for _ in range(NR_CHUNKS):
                data = await server.recv(bufsize=CHUNK_SIZE)
                await server.send(data=data)

    async def client_loop():
        for _ in range(ITERATIONS):
            for chunk in DATA_CHUNKS:
                await client.send(data=chunk)
            for _ in range(NR_CHUNKS):
                await client.recv(bufsize=CHUNK_SIZE)

    server_task = asyncio.create_task(server_loop())
    start = time.perf_counter()
    await client_loop()
    elapsed = time.perf_counter() - start

    await server_task
    await server.close()
    await client.close()
    return elapsed


# ---------------------------------------------------------------------
# 6. puring vectored: 1 sendmsg()/recvmsg() SQE per iteration.
# ---------------------------------------------------------------------

async def puring_vectored_udp():
    print('Running puring vectored UDP sendmsg/recvmsg (io_uring)')
    server_port, client_port = PORTS['puring_vectored']

    server = await puring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)
    await server.bind(host=HOST, port=server_port)
    await server.connect(host=HOST, port=client_port)

    client = await puring.prep_socket(domain=socket.AF_INET, socktype=socket.SOCK_DGRAM)
    await client.bind(host=HOST, port=client_port)
    await client.connect(host=HOST, port=server_port)

    async def server_loop():
        bufs = _make_bufs()
        for _ in range(ITERATIONS):
            await server.recvmsg(buffers=bufs)
            await server.sendmsg(buffers=bufs)

    async def client_loop():
        bufs = _make_bufs()
        for _ in range(ITERATIONS):
            await client.sendmsg(buffers=DATA_CHUNKS)
            await client.recvmsg(buffers=bufs)

    server_task = asyncio.create_task(server_loop())
    start = time.perf_counter()
    await client_loop()
    elapsed = time.perf_counter() - start

    await server_task
    await server.close()
    await client.close()
    return elapsed


def run():
    total_mb = TOTAL / (1024 ** 2)
    print(
        f'{CHUNK_SIZE=}, {NR_CHUNKS=}, {ITERATIONS=}, '
        f'datagram={CHUNK_SIZE * NR_CHUNKS} bytes, one-way total={total_mb:.0f} MiB'
    )

    results = []

    t = sync_scalar_udp()
    results.append(('stdlib scalar', t))

    t = sync_vectored_udp()
    results.append(('stdlib vectored', t))

    t = asyncio.run(asyncio_vectored_udp())
    results.append(('asyncio vectored', t))

    if is_uvloop_installed:
        uvloop.install()
        t = asyncio.run(uvloop_vectored_udp())
        results.append(('uvloop vectored', t))

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        t = runner.run(puring_scalar_udp())
        results.append(('puring scalar', t))

        t = runner.run(puring_vectored_udp())
        results.append(('puring vectored', t))

    print('\n==== RESULTS ====')
    for name, sec in results:
        print(f'{name:18s} {mbps(sec):10.2f} MB/s   ({sec:.3f}s)')


if __name__ == '__main__':
    run()
