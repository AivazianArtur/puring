# Puring: High-Performance io_uring wrapper for Python

## Why Puring?
* **True Async File I/O:** Unlike epoll-based `asyncio` and `uvloop`, `puring` is based on io_uring, which provides real async I/O without thread pools for files \
<small> For full explanation, go [here](docs/uring/EPOLL_VS_URING.md) </small>
* **Seamless Integration:** Designed to work as a plug-in for the standard `asyncio` event loop.
* **Low Overhead:** C-implemented request registry with $O(1)$ lookup.
* **Simple Architecture:** Simple layered architecture that allows to easy understanding of what`s going on
* **Full io_uring support** In future we'll implement all io_uring features

## Architecture
### Why C-Python API
The question comes down to choosing between this and Cython.
Main reason to use C-Python API - it has maximum potential regarding speed. \
Learning curve to understand C code for Python programmer is tougher than CPython, but it pays a lot with understanding Python, memory management and where is the line between C and Python actually lays. \
CFFI is not an option because based on this we can`t get usable API.

### Why Python needs it
It brings proactor pattern to Python in Linux, that:
* Allows implementation of async fie I/O operations. 
* Supports modern Python way of getting rid of GIL.

### Current State
* Core C-engine for Ring management.
* Registry-based request tracking to connect futures with their result from CQE.
* Python C-API bridge for `asyncio.Future` resolution.
* Basic file usage. Brings true Async I/O.
* Basic socket usage.

### How it works
To read about implementation details, go to [architecture page](docs/ARCHITECTURE.md)


## Benchmarks
Now we have just simple benchmarks, showing that even in pre-alpha mode and with many features to come, it is already faster while working with files.

To learn more, go to [benchmarks documentation](docs/BENCHMARK.md)


## Contributing
To start contribute, go to our [contributing guideline](docs/guidelines/CONTRIBUTING.md)

We are looking for help with:
1. Testing on different Linux Distros/Kernels.
2. Sharing experience in memory management, libraries architecture and many other things
3. Write tests and benchmarks \
And many more, see our [roadmap](docs/ROADMAP.md) \

## Using
### Developer
To install, go to [installation page](docs/guidelines/INSTALLATION.md) \
To start contribute, go to [developer guideline](docs/guidelines/DEVELOPING.md) and [contribution guideline](docs/guidelines/CONTRIBUTING.md)

### User
See how to use here - [usage guide](docs/USAGE.md)


## Roadmap
See [here](docs/ROADMAP.md)
