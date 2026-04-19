# Roadmap

## v0.2.0 - Stabilize current implementation
- [X] Polish current implementation:
    - [X] Debug sockets and add simple benchmark.
    - [X] Implement `ASSERT_LOOP_THREAD` macros.
    - [X] Replace `peek_cqe` with `wait_cqe_timeout` on app shutdown and loop closing.
    - [X] Implement `UringFile` to fix file API and shadow FD usage.
    - [X] Move initialization to separate module.
    - [X] Improve separation between C and Python layers.

## v0.3.0 - Core features
- [X] Translate internal errors into native Python exceptions.
- [X] Add timer support (`IORING_OP_TIMEOUT`).
- [X] Add signal handling (`SIGINT`, `SIGTERM`, `SIGQUIT`) via `IORING_OP_POLL_ADD`.
- [X] Add not implemented `file` methods.
- [ ] Add not implemented `socket` methods.
- [ ] Finish AbstractEventLoop Protocol.

## v0.3.5 - Buffer features
- [ ] Implement fixed buffers (`io_uring_register_buffers`).
- [ ] Implement zero-copy send (`send_zc`).
- [ ] Use Python application buffers as `UringLoop` buffers.
- [ ] Implement a puring-based memory pool for buffers.
- [ ] Implement provided buffer rings / multishot (`register_pbuf_ring`).

## v0.4.0 - Async Runtime & Architecture
- [ ] Run reader in separate thread.
- [ ] Each thread must own it`s own Rings
- [ ] Multiring support.
- [ ] Support all awaitable operations.
- [ ] Support modern Python async interfaces (`Runners`).
- [ ] Configurable ring parameters.
- [ ] Revisit loop replacement (currently uncertain but likely needed).
- [ ] Improve future creation: `FutureFactory` and `future pool`.
- [ ] `Batcher` and `Chainer` OOP interface.
- [ ] Implement `dir` object completely.

## v0.5.0 - CQE and SQE production features
- [ ] CQE batching.
- [ ] SQPOLL.
- [ ] FASTPOLL.
- [ ] Support linked SQEs for chained operations.
- [ ] Support `IOSQE_CQE_SKIP_SUCCESS` and `IOSQE_BUFFER_SELECT`.
- [ ] Keep file descriptors registered (`io_uring_register_files`).


## v0.5.1 - Community & Contribution
- [ ] Add tests.
- [ ] Implement GitHub CI.
- [ ] Publish library on PyPI (`pip` installable).
- [ ] Document everything.
- [ ] Add contribution support:
    - [ ] Write contribution guidelines.
    - [ ] Add repository badges.
    - [ ] Enable GitHub issue tracking.


### Note
This roadmap is expected to evolve as our understanding deepens and the project matures.
