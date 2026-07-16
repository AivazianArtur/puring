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
- [X] Add not implemented `socket` methods.
- [X] Finish AbstractEventLoop Protocol.
- [X] Implement GitHub CI.

## v0.4.0 - Buffer features
- [ ] Implement `Execution Context` layer: `Buffer Mode`, `Transfer Mode` and `Stream Strategy`.
    - [X] Implement `Buffer Mode` layer:
        - [X] Implement `fixed` buffer mode.
        - [X] Implement `provided` and `buffer ring` buffer modes.
        - [X] Use Python application buffers as `PuringLoop` buffers - `NORMAL_BUF` buffer mode.
        - [X] Implement modes dispatching.
    - [X] Implement `Transfer Mode`:
        - [X] Zero-copy functionality.
        - [X] Zero-copy and normal-copy routing.
    - [ ] Implement `Stream Strategy` layer:
        - [ ] Multishot functionality.
        - [ ] Multishot and one-shot(normal) routing.
- [ ] Implement `ContextManager` protocol for Files and Sockets.
- [ ] Use `errno` in error handling.
- [ ] Add tests for whole library:
    - [ ] Python-layer.
    - [ ] C-layer.
- [ ] Publish library on PyPI (`pip` installable).

## v0.5.0 - Async Runtime & Architecture
- [ ] Run reader in separate thread.
- [ ] Each thread must own it`s own Rings
- [ ] Multiring support.
- [ ] Support all awaitable operations.
- [ ] Support modern Python async interfaces (`Runners`).
- [ ] Configurable ring parameters.
- [ ] Improve future creation: `FutureFactory` and `future pool`.
- [ ] `Batcher` and `Chainer` OOP interface.
- [ ] Implement `dir` object completely.
- [ ] Research and implement futex if needed.
- [ ] Add poll interface (with multishot poll).
- [ ] Add `socket.send` with `IORING_SEND_VECTORIZED` and `IORING_RECVSEND_BUNDLE` flags as separate methods.
- [ ] Add zero-copy RX interface.

## v0.6.0 - CQE and SQE production features
- [ ] CQE batching.
- [ ] SQPOLL.
- [ ] FASTPOLL.
- [ ] Support linked SQEs for chained operations.
- [ ] Support `IOSQE_CQE_SKIP_SUCCESS` and `IOSQE_BUFFER_SELECT`.
- [ ] Keep file descriptors registered (`io_uring_register_files`).
- [ ] Persistent log of CQ and SQ entry for backuping.
- [ ] Explore event loop tick cycles variants.

## v0.6.1 - Community & Contribution
- [ ] Document everything.
- [ ] Add contribution support:
    - [ ] Write contribution guidelines.
    - [ ] Add repository badges.
    - [ ] Enable GitHub issue tracking.

## v0.7.0 - Additional functionality
- [ ] Ancillary data support for sockets scatter-gather.
- [ ] Support io_uring_prep_cmd_sock.

### Note
This roadmap is expected to evolve as our understanding deepens and the project matures.
