# Roadmap

## v0.2 - Stabilize current implementation
- [ ] Polish current implementation:
    - [X] Debug sockets and add simple benchmark.
    - [ ] Implement `ASSERT_LOOP_THREAD` macros.
    - [X] Replace `peek_cqe` with `wait_cqe_timeout` on app shutdown and loop closing.
    - [ ] Implement `UringFile` to fix file API and shadow FD usage.
    - [X] Move initialization to separate module.
    - [X] Improve separation between C and Python layers.

## v0.3 - Core features
- [ ] Translate internal errors into native Python exceptions.
- [ ] Proper GIL handling.
- [ ] Add timer support (`IORING_OP_TIMEOUT`).
- [ ] Add signal handling (`SIGINT`, `SIGTERM`) via `IORING_OP_POLL_ADD`.
- [ ] Add not implemented `loop`, `file` and `socket` methods.
- [ ] Add tests.
- [ ] Implement GitHub CI.

## v0.4 - Performance io_uring features
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

## v0.8 - Community & Contribution
- [ ] Publish library on PyPI (`pip` installable).
- [ ] Document everything.
- [ ] Add contribution support:
    - [ ] Write contribution guidelines.
    - [ ] Add repository badges.
    - [ ] Enable GitHub issue tracking.

---

### Note
This roadmap is expected to evolve as our understanding deepens and the project matures.
