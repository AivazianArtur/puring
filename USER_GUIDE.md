#### See more examples in [docs/examples](docs/examples)

## Installation
First off, install library with
> pip install puring

## Initialize `PuringLoop`
To start using `puring` user need to initialize special `io_uring` loop - `PuringLoop` \
Its quite simple:
> asyncio.run(main(), loop_factory=puring.PuringLoop)

Or with Runner:
```python
with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
    runner.run(main())
```

## `File` and `Socket` Operations
### `File` operations
`puring.File` object acts as interface to true asynchronous file operations in Python.
**See full File API TODO: [locally]() and [remotely]()**

You can import it by
> from puring import File

Let's create simple example of writing and reading from file:
1. Open file asynchronously
> file = await puring.open_file(path='path/to/file.txt')
2. Write to file, asynchronously. Better use bytes in v0.4.0
> await file.write(data=b'Hello puring!\n')
3. Read from file, again - asynchronously
> result_data = await file.read()
4. Closing file asynchronously 
> await file.close()

Now lets put it inside function:
```python
async def file_simple_example():
    file = await puring.open_file(path=TEMPFILE)
    await file.write(data=b'Hello, puring!\n')
    await file.read()
    await file.close()
```

**Important note** - you can await this function only with new `PuringLoop` loop. \
See how easy it is actually to combine everything we know so far:
```python
import puring

async def file_simple_example():
    file = await puring.open_file(path=TEMPFILE)
    await file.write(data=b'Hello, puring!\n')
    await file.read()
    await file.close()

asyncio.run(file_simple_example(), loop_factory=puring.PuringLoop)
```

But actual code would be even simpler - with usage of file context manager
```python
import puring

async def file_simple_example():
    async with await puring.open_file(path=TEMPFILE) as file:
        await file.write(data=b'Hello, puring!\n')
        await file.read()

asyncio.run(file_simple_example(), loop_factory=puring.PuringLoop)
```
You can also use `asyncio.Runner` when you need more control over the event loop:
```python
with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
    runner.run(file_simple_example())
```

### `Socket` operations
`puring.Socket` object acts as interface to asynchronous socket operations in Python using `io_uring`, not `epoll`.
**See full File API TODO: [locally]() and [remotely]()**

You can import it by
> from puring import Socket

Let's create simple example of sending message from one socket to another:
1. Prepare socket
> socket = await puring.prep_socket()
2. Bind to socket
> await socket.bind('127.0.0.1', 12878)
3. Listen socket
> await socket.listen()
4. Accept an incoming connection
> connection = await socket.accept()
5. Receive data from the connection
> await connection.recv()
6. Send data through a socket
> await socket.send(data)
7. Close the socket asynchronously
> await socket.close()

Now let's put everything together into a simple client-server example,
As with file operations, socket operations must be executed using the `PuringLoop`:

```python
import puring

async def socket_simple_example():
    server_socket = await puring.prep_socket()
    await server_socket.bind('127.0.0.1', 12878)
    await server_socket.listen()

    client_socket = await puring.prep_socket()

    await client_socket.connect('127.0.0.1', 12878)

    server_connection = await server_socket.accept()
    await client_socket.send(b'Hello from client!')

    received_data = await server_connection.recv()

    await client_socket.close()
    await server_connection.close()
    await server_socket.close()

asyncio.run(socket_simple_example(), loop_factory=puring.PuringLoop)
```
The example above creates two sockets: one for the server and one for the client. \
The server socket is bound to `127.0.0.1:12878` and starts listening for incoming connections. \
The client then connects to the server, after which the server accepts the connection and receives the message sent by the client.

You can also use `asyncio.Runner` when you need more control over the event loop:
```python
with asyncio.Runner(loop_factory=puring.PuringLoop) as runner:
    runner.run(socket_simple_example())
```

And there is support of context manager protocol:
```python
import puring

async def socket_simple_example():
    async with await puring.prep_socket() as server_socket:
        await server_socket.bind('127.0.0.1', 12878)
        await server_socket.listen()

        async with await puring.prep_socket() as client_socket:
            await client_socket.connect('127.0.0.1', 12878)

            async with await accept_future as server_connection:
                await client_socket.send(b'Hello from client!')
                received_data = await server_connection.recv()

asyncio.run(socket_simple_example(), loop_factory=puring.PuringLoop)
```

## Execution Context
To use advanced features, use different types of context managers.\
**Note that not all operations are supported in every context, but it will automatically dispatched to supported operation**

### TransferMode
To use operations in zero-copy mode, use `PuringLoop.transfer_mode` context manager, with `puring.TRANSFER_MODE` Enum:
```python
async def transfer_mode():
    loop = asyncio.get_running_loop()
    with loop.transfer_mode(mode=puring.TRANSFER_MODE.ZERO_COPY):
        await simple_socket_example()

asyncio.run(transfer_mode(), loop_factory=puring.PuringLoop)
```
There is `NORMAL` and `ZERO_COPY` values in `TRANSFER_MODE` Enum

### StreamStrategy
To use Multishot operations, use `stream_strategy` context manager, with `puring.STREAM_STRATEGY` Enum. In v0.4.0 its better to not use it: 
```python
async def stream_strategy():
    loop = asyncio.get_running_loop()
    with loop.stream_strategy(stream=puring.STREAM_STRATEGY.MULTISHOT):
        await simple_socket_example()

asyncio.run(stream_strategy(), loop_factory=puring.PuringLoop)
```
There is `ONESHOT` and `MULTISHOT` values in `STREAM_STRATEGY` Enum

### BufferMode
To use operations in zero-copy mode, use `PuringLoop.buffer_mode` context manager with `puring.BUFFER_MODE` Enum:
#### Fixed buffer mode:
**Note, FIXED buffer mode currently working only with files**
```python
async def buffer_mode():
    loop = asyncio.get_running_loop()
    buf = bytearray(4096)
    with loop.buffer_mode(mode=puring.BUFFER_MODE.FIXED, buffers=[buf]):
        await simple_file_example()

asyncio.run(buffer_mode(), loop_factory=puring.PuringLoop)
```

There is `NORMAL`, `FIXED`, `PROVIDED` and `BUF_RING` values in `BUFFER_MODE` Enum

### ExecutionContext
To set context manager with different types of all regimes above - use `PuringLoop.execution_context` context manager:
```python
async def execution_context():
    loop = asyncio.get_running_loop()

    buf = bytearray(4096)
    with loop.execution_context(
        stream_strategy=puring.STREAM_STRATEGY.MULTISHOT,
        buffer_mode=puring.BUFFER_MODE.FIXED,
        transfer_mode=puring.TRANSFER_MODE.ZERO_COPY,
        buffers=[buf],
    ):
        await simple_file_example()

asyncio.run(buffer_mode(), loop_factory=puring.PuringLoop)
```

If you face some problems, please leave an issue on [Github](https://github.com/AivazianArtur/puring/issues)
