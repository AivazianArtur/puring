# Puring: High-Performance io_uring wrapper for Python

## Why Puring?
* **True Async File I/O:** Unlike epoll-based `asyncio` and `uvloop`, `puring` is based on io_uring, which provides real async I/O without thread pools for files \
<small> For full explanation, go [here](docs/uring/URING.md) </small>
* **Seamless Integration:** Designed to work as a plug-in for the standard `asyncio` event loop.
* **Low Overhead:** C-implemented request registry with $O(1)$ lookup.
* **Simple Architecture:** Simple layered architecture that makes it easy to understand of what's going on.
* **Full io_uring support** In future we'll implement all io_uring features.
* **C-Python API** Implemented using CPython C-API for minimal overhead and full control over memory and GIL behavior.

## Quick Example:
``` 
async def main():
    loop = puring.uring(registry_size=8)

    puring.add_uring_reader(loop)
    fd = await loop.open(path='testfile.txt')

    data = b'Hello, puring!\n'
    await loop.write(fd, data=data)
    await loop.read(fd=fd)

    await  loop.close(fd=fd)

    loop.close_loop()

asyncio.run(main())
```

## Comparison
| Feature             | asyncio      | uvloop       | puring   |
| ------------------- | ------------ | ------------ | -------- |
| Async files         | ❌ threadpool | ❌ threadpool | ✅ native |
| Syscalls            | many         | many         | minimal  |
| Kernel batching     | ❌            | ❌            | ✅        |
| Zero-copy potential | ❌            | ❌            | ✅        |


## Quick Install
#### Warning! Linux only
> git clone git@github.com:AivazianArtur/puring.git \
> cd puring \
> make install

## Architecture
### Why Python needs it
It brings proactor pattern to Python in Linux, that:
* Allows implementation of async file I/O operations. 
* Enables designs compatible with upcoming no-GIL Python efforts.

### Current State
* Core C-engine for Ring management.
* Registry-based request tracking to connect futures with their result from CQE.
* Python C-API bridge for `asyncio.Future` resolution.
* Basic file usage. Brings true Async I/O.
* Basic socket usage.

### Goal
* Progressive coverage of io_uring features.
### How it works
To read about implementation details, go to [architecture page](docs/ARCHITECTURE.md)


## Benchmarks
Now we have just simple benchmarks, showing that even in pre-alpha mode and with many features to come, it is already faster while working with files.
This is result:

| Chunk Size | Chunk Size (KiB) | Standard (MB/s) | Asyncio Thread (MB/s) | Uvloop Thread (MB/s) | io_uring Seq (MB/s) |
|-----------:|-----------------:|----------------:|----------------------:|---------------------:|--------------------:|
| 1048576 | 1024 | 72.30 | 64.75 | 73.58 | 137.02 |
| 524288  | 512  | 65.38 | 56.00 | 61.89 | 128.94 |
| 262144  | 256  | 56.98 | 47.85 | 53.70 | 112.57 |
| 131072  | 128  | 46.69 | 41.96 | 49.11 | 101.51 |
| 65536   | 64   | 45.97 | 36.26 | 43.65 | 78.05 |
| 32768   | 32   | 46.60 | 32.54 | 43.00 | 66.72 |
| 16384   | 16   | 35.24 | 24.44 | 31.24 | 36.25 |
| 8192    | 8    | 25.36 | 17.07 | 21.55 | 18.37 |


To learn more, go to [benchmarks documentation](docs/BENCHMARK.md)


## Contributing
To start contribute, go to our [contributing guideline](docs/guidelines/CONTRIBUTING.md)

We are looking for help with:
1. Testing on different Linux Distros/Kernels.
2. Sharing experience in memory management, libraries architecture and many other things
3. Write tests and benchmarks \
And many more, see our [roadmap](docs/ROADMAP.md)

## Using
### Developer
To install, go to [installation page](docs/guidelines/INSTALLATION.md) \
To start contribute, go to [developer guideline](docs/guidelines/DEVELOPING.md) and [contribution guideline](docs/guidelines/CONTRIBUTING.md)

### User
See how to use here - [usage guide](docs/USAGE.md)


## Roadmap
See [here](docs/ROADMAP.md)
