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

# ---- Config -----------------------------------------------------------
# "scalar"   variant: NR_CHUNKS separate datagrams, one syscall/SQE each.
# "vectored" variant: 1 datagram assembled from NR_CHUNKS buffers via
#                      sendmsg (gather) / recvmsg (scatter).
#
# Keep NR_CHUNKS * CHUNK_SIZE comfortably under ~64 KiB - that's the
# practical UDP datagram ceiling, even on loopback.
#
# ---- ON THE TWO DIFFERENT "asyncio/uvloop" VARIANTS BELOW -------------
# There are two fundamentally different things you can measure when you
# put a blocking socket call behind asyncio, and they answer different
# questions:
#
#   BATCH   (*_batch_udp):    the ENTIRE ITERATIONS loop is handed to a
#            single asyncio.to_thread() call. The event loop pays the
#            thread-dispatch cost exactly ONCE for the whole benchmark,
#            then the loop runs as plain blocking C-level socket calls
#            in a background thread with no event-loop involvement at
#            all in between. This measures "how fast is the raw
#            syscall path if you get to ignore the event loop", which
#            is a real and useful number, but it is NOT a per-operation
#            concurrency model - nothing here is comparable to puring,
#            which submits one SQE per sendmsg/recvmsg call.
#
#   DISPATCH (*_dispatch_udp): each individual send/recv/sendmsg/recvmsg
#            call gets its own asyncio.to_thread() call - one thread
#            hop per operation, same granularity as puring's one SQE
#            per operation. This is the actual apples-to-apples
#            comparison: "cost of a thread-pool hop per op" vs "cost of
#            an io_uring SQE per op".
#
# Comparing puring against the BATCH variants is comparing it to
# something that pays dispatch overhead once instead of N times - of
# course that wins on cheap ops. The DISPATCH variants are the fair
# fight; keep the BATCH numbers around only as an upper-bound reference
# for "what if there were no event loop at all".

CHUNK_SIZE = 4 * 1024
NR_CHUNKS = 8
ITERATIONS = 2000

TOTAL = CHUNK_SIZE * NR_CHUNKS * ITERATIONS
DATA_CHUNKS = [bytes([i % 256]) * CHUNK_SIZE for i in range(NR_CHUNKS)]

HOST = '127.0.0.1'

# distinct port pairs per backend/mode so runs never collide
PORTS = {
    'stdlib_scalar':            (9301, 9311),
    'stdlib_vectored':          (9302, 9312),
    'asyncio_vectored_batch':   (9303, 9313),
    'uvloop_vectored_batch':    (9304, 9314),
    'puring_scalar':            (9305, 9315),
    'puring_vectored':          (9306, 9316),
    'asyncio_scalar_dispatch':  (9307, 9317),
    'uvloop_scalar_dispatch':   (9308, 9318),
    'asyncio_vectored_dispatch': (9309, 9319),
    'uvloop_vectored_dispatch': (9310, 9320),
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


def _make_bufs():
    return [bytearray(CHUNK_SIZE) for _ in range(NR_CHUNKS)]


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
# 3/4. BATCH variants: whole ITERATIONS loop behind ONE to_thread() call.
#    Kept as an "event loop out of the picture entirely" reference -
#    NOT comparable to puring's per-op SQE model. See the big comment
#    at the top of the file.
# ---------------------------------------------------------------------

async def _batch_vectored_udp(server_port, client_port):
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


async def asyncio_vectored_batch_udp():
    print('Running asyncio BATCH vectored UDP (one to_thread for the whole loop)')
    return await _batch_vectored_udp(*PORTS['asyncio_vectored_batch'])


async def uvloop_vectored_batch_udp():
    print('Running uvloop BATCH vectored UDP (one to_thread for the whole loop)')
    return await _batch_vectored_udp(*PORTS['uvloop_vectored_batch'])


# ---------------------------------------------------------------------
# 5/6. DISPATCH variants: one to_thread() call PER send/recv (scalar)
#    or PER sendmsg/recvmsg (vectored) - the same granularity as
#    puring's one-SQE-per-call model. This is the fair comparison.
# ---------------------------------------------------------------------

async def _dispatch_scalar_client_loop(client):
    for _ in range(ITERATIONS):
        for chunk in DATA_CHUNKS:
            await asyncio.to_thread(client.send, chunk)
        for _ in range(NR_CHUNKS):
            await asyncio.to_thread(client.recv, CHUNK_SIZE)


async def _dispatch_vectored_client_loop(client):
    bufs = _make_bufs()
    for _ in range(ITERATIONS):
        await asyncio.to_thread(client.sendmsg, DATA_CHUNKS)
        await asyncio.to_thread(client.recvmsg_into, bufs)


async def _dispatch_scalar_udp(server_port, client_port):
    server, client = make_udp_pair(server_port, client_port)

    # server side isn't timed, so it can stay as a plain background
    # thread without affecting the measured (client-side) cost, as
    # long as it can keep up with the client's request rate.
    server_fut = asyncio.ensure_future(
        asyncio.to_thread(_scalar_server_loop, server)
    )

    start = time.perf_counter()
    await _dispatch_scalar_client_loop(client)
    elapsed = time.perf_counter() - start

    await server_fut
    server.close()
    client.close()
    return elapsed


async def _dispatch_vectored_udp(server_port, client_port):
    server, client = make_udp_pair(server_port, client_port)

    server_fut = asyncio.ensure_future(
        asyncio.to_thread(_vectored_server_loop, server)
    )

    start = time.perf_counter()
    await _dispatch_vectored_client_loop(client)
    elapsed = time.perf_counter() - start

    await server_fut
    server.close()
    client.close()
    return elapsed


async def asyncio_scalar_dispatch_udp():
    print('Running asyncio DISPATCH scalar UDP (to_thread per send/recv call)')
    return await _dispatch_scalar_udp(*PORTS['asyncio_scalar_dispatch'])


async def uvloop_scalar_dispatch_udp():
    print('Running uvloop DISPATCH scalar UDP (to_thread per send/recv call)')
    return await _dispatch_scalar_udp(*PORTS['uvloop_scalar_dispatch'])


async def asyncio_vectored_dispatch_udp():
    print('Running asyncio DISPATCH vectored UDP (to_thread per sendmsg/recvmsg call)')
    return await _dispatch_vectored_udp(*PORTS['asyncio_vectored_dispatch'])


async def uvloop_vectored_dispatch_udp():
    print('Running uvloop DISPATCH vectored UDP (to_thread per sendmsg/recvmsg call)')
    return await _dispatch_vectored_udp(*PORTS['uvloop_vectored_dispatch'])


# ---------------------------------------------------------------------
# 7. puring scalar: NR_CHUNKS separate send()/recv() SQEs per iteration.
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
# 8. puring vectored: 1 sendmsg()/recvmsg() SQE per iteration.
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

    t = asyncio.run(asyncio_vectored_batch_udp())
    results.append(('asyncio vectored BATCH', t))

    t = asyncio.run(asyncio_scalar_dispatch_udp())
    results.append(('asyncio scalar DISPATCH', t))

    t = asyncio.run(asyncio_vectored_dispatch_udp())
    results.append(('asyncio vectored DISPATCH', t))

    if is_uvloop_installed:
        uvloop.install()

        t = asyncio.run(uvloop_vectored_batch_udp())
        results.append(('uvloop vectored BATCH', t))

        t = asyncio.run(uvloop_scalar_dispatch_udp())
        results.append(('uvloop scalar DISPATCH', t))

        t = asyncio.run(uvloop_vectored_dispatch_udp())
        results.append(('uvloop vectored DISPATCH', t))

    with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
        t = runner.run(puring_scalar_udp())
        results.append(('puring scalar', t))

        t = runner.run(puring_vectored_udp())
        results.append(('puring vectored', t))

    print('\n==== RESULTS ====')
    print('(BATCH = one to_thread for the whole loop, not comparable to')
    print(' puring\'s per-op SQE model - see comment at top of file.')
    print(' DISPATCH = one to_thread per op, the fair comparison.)\n')
    for name, sec in results:
        print(f'{name:28s} {mbps(sec):10.2f} MB/s   ({sec:.3f}s)')


if __name__ == '__main__':
    run()
