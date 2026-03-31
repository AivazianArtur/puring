# Roadmap

## v0.2 - Stabilize current implementation
- [X] Polish current implementation:
    - [X] Debug sockets and add simple benchmark.
    - [X] Implement `ASSERT_LOOP_THREAD` macros.
    - [X] Replace `peek_cqe` with `wait_cqe_timeout` on app shutdown and loop closing.
    - [X] Implement `UringFile` to fix file API and shadow FD usage.
    - [X] Move initialization to separate module.
    - [X] Improve separation between C and Python layers.

## v0.3 - Core features
- [X] Translate internal errors into native Python exceptions.
- [ ] Add timer support (`IORING_OP_TIMEOUT`).
- [ ] Add signal handling (`SIGINT`, `SIGTERM`) via `IORING_OP_POLL_ADD`.
- [ ] Finish AbstractEventLoop Protocol.
- [ ] Add not implemented `file` and `socket` methods.
- [ ] Add tests.
- [ ] Implement GitHub CI.

## v0.4 - Buffer features
- [ ] Implement fixed buffers (`io_uring_register_buffers`).
- [ ] Implement zero-copy send (`send_zc`).
- [ ] Use Python application buffers as `UringLoop` buffers.
- [ ] Implement a puring-based memory pool for buffers.
- [ ] Implement provided buffer rings / multishot (`register_pbuf_ring`).

## v0.6 - CQE and SQE production features
- [ ] CQE batching.
- [ ] SQPOLL.
- [ ] FASTPOLL.
- [ ] Support linked SQEs for chained operations.
- [ ] Keep file descriptors registered (`io_uring_register_files`).

## v0.7 - Async Runtime & Architecture
- [ ] Multiring support.
- [ ] Support all awaitable operations.
- [ ] Support modern Python async interfaces (`Runners`).
- [ ] Configurable ring parameters.
- [ ] Revisit loop replacement (currently uncertain but likely needed).
- [ ] Improve future creation: `FutureFactory` and `future pool`.

## v0.7.1 - Community & Contribution
- [ ] Publish library on PyPI (`pip` installable).
- [ ] Document everything.
- [ ] Add contribution support:
    - [ ] Write contribution guidelines.
    - [ ] Add repository badges.
    - [ ] Enable GitHub issue tracking.

## v0.8 - Read Completion Queue in separate thread per process
- [ ] Run reader in separate thread.

## v0.9 - Rings per Thread
- [ ] Each thread must own it`s own Rings
---

### Note
This roadmap is expected to evolve as our understanding deepens and the project matures.
