# Puring: High-Performance io_uring wrapper for Python

## Why Puring?
* **True Async File I/O:** Unlike epoll-based loops, Puring uses native Linux `io_uring` for non-blocking file operations without thread pools.
* **Low Overhead:** C-implemented request registry with $O(1)$ lookup.
* **Seamless Integration:** Designed to work as a plug-in for the standard `asyncio` event loop.

## Architecture
### Why C-Python API
The question comes down to choosing between this and Cython.
Main reason to use C-Python API - it has maximum potential regarding speed. \
Learning curve to understand C code for Python programmer is tougher than CPython, but it pays a lot with understanding Python, memory management and where is the line between C and Python actually lays. \
CFFI is not an option because based on this we can`t get usable API.


### Current State
* Core C-engine for Ring management.
* Registry-based request tracking.
* Python C-API bridge for `asyncio.Future` resolution.
* Basic file usage. It is really async file write in Python.
* Basic socket usage 

### Roadmap
* [ ] Support

## Benchmarks
Now we have just simple benchmarks, showing us that even in pre-alpha mode and with many features to come, it is already faster while working with files.

To learn more, go to [benchmarks documentation](docs/BENCHMARK.md)

## Contributing
We are looking for help with:
1. Testing on different Linux Distros/Kernels.
2. Sharing experience in memory management, libraries architecture and many other things
3. Write tests and benchmarks
4. and many more, see our [roadmap](docs/ROADMAP.md)


## Using
### Developer
To install, go here - [Installation](docs/guidelines/INSTALLATION.md) \
To start contribute, go to [developer guideline](docs/guidelines/DEVELOPING.md) and [contribution guideline](docs/guidelines/CONTRIBUTING.md)
### User
See how to use here - [Usage](docs/USAGE.md)
