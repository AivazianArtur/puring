## Principles:
  - Strong layers between Python interface and C-functionality
  - Trying to stick to DDD principles

### Structure
Structure of this project is trying to be simple - we have pure c-layer and pure python-layer and layer in between.
- C-layer - Functionality written entirely in C and basically wrappers aroung liburing ABI
- Python-layer - By analogy it is layer, written with usage of CPython API and CPython objects.
- Layer in between - for now here is really only one thing - registry. It is container to hold objects in between. I wish i could say that in between C-layer and Python-layer, but the meaning is not so. We are working with async nature and giving control of the operations to kernel, so we can do our things. To map the result of kernel with what was intended we need some sort of storage - this is registry. 

## Introduction
### io_uring
Puring is written natively in CPython and brings the new event loop, based on io_uring.
io_uring is an alternate for 

What is io_uring and how it works, explained for Python developers - [here](TODO)

### Domains

1. Ring
2. Event loop
3. Reader
4. OPS
5. Buffer
6. ExecutionContext
7. Registry
8. Timer

Main challenge was to connect Python event loop and io_uring.

1. From io_uring side main thing is `ring` itself. Ring contains two rings actually - `Submission Queue` with `SQE` and `Completion Queue` with `CQE`(`E` is from `event`). It really important to understand this concepts, but it all really gives us only one domain - `ring`.

2. From python side there is main object too - `Event Loop`. Loop is complicated, and its complicity is shown in code: by `loop` domain with `PuringLoop` object and by `reader` domain.

3. `Reader` is part of loop that reads result of I/O multiplexing mechanism(the output part). In our case it reads directly result of operations, but when you work with `epoll` its different.

4. Next domain is `OPS`, which contains `File` and `Socket` objects. This domain is mirrorly presented in both c-layer and python-layer. Its purpose is to be an API of system operations.

5. To read and write from/to Files and Sockets, we need buffers. And there is separate `Buffer` domain for this purpose.

6. But also io_uring gives us some workarounds for buffers, for example `PROVIDED` buffer mode, where you allocating buffers for io_uring and than io_uring controlls them, not you. There are more `Buffer Modes`, and there is also two another dimensions of modding ops - `Stream Strategy` and `Transfer Mode`. All this are parts of `ExecutionContext` Domain.

7. `Registry` is storage of operations, to map them in `Reader`.

8. `Timer` - as we are waiting for kernel to done the operation, we can set timeouts. That is purpose of this layer.


### New python objects
- Main:
    - PuringLoop
    - File
    - Socket
- Helpers:
    - BufferModeCtx
    - StreamStrategyCtx
    - TransferModeCtx
    - ExecutionContextCtx
- Enums:
    - BUFFER_MODE
    - STREAM_STRATEGY
    - TRANSFER_MODE
    - PAYLOAD_TYPE
    - Resolve Flags
    - Statx Flags
    - StatxMask


## Domains

## Python objects

#NOTE While PuringLoop is child of BaseEventLoop, PuringSocket and PuringFile are build from scratch.
