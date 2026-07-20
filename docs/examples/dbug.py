import sys
sys.path.insert(0, '')
import asyncio
import gc
import weakref
import puring

HOST = '127.0.0.1'
PORT = 12878


async def transfer_mode():
    loop = asyncio.get_running_loop()
    with loop.transfer_mode(mode=puring.TRANSFER_MODE.ZERO_COPY):
        server_sock = await puring.prep_socket()
        await server_sock.bind(HOST, PORT)
        await server_sock.listen(1)

        accept_future = server_sock.accept()
        client_sock = await puring.prep_socket()
        await client_sock.connect(HOST, PORT)

        server_conn = await accept_future
        message = b'Hello from client!'
        await client_sock.send(message)

        received_data_r = server_conn.recv()
        received_data = await received_data_r
        print(f'Server received: {received_data.decode()}')

        await client_sock.close()
        await server_conn.close()
        await server_sock.close()


if __name__ == '__main__':
    captured = {}

    def loop_factory():
        loop = puring.PuringLoop()
        captured['loop'] = loop
        return loop

    asyncio.run(transfer_mode(), loop_factory=loop_factory)

    loop = captured.pop('loop')
    loop_ref = weakref.ref(loop)

    gc.collect()

    print('=== referrers BEFORE del loop ===')
    for r in gc.get_referrers(loop):
        print(' -', type(r), repr(r)[:200])

    del loop
    gc.collect()

    print('=== loop alive after del+collect:', loop_ref() is not None)
    if loop_ref() is not None:
        print('=== referrers AFTER del loop (real leak source) ===')
        for r in gc.get_referrers(loop_ref()):
            print(' -', type(r), repr(r)[:300])
            # если это frame/cell/dict - копнём глубже
            for rr in gc.get_referrers(r):
                print('     held by:', type(rr), repr(rr)[:200])
                