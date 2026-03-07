# Roadmap

## Phase 1 — Stabilize Current Implementation
- [ ] Polish current implementation:
    - [ ] Implement `ASSERT_LOOP_THREAD` macros.
    - [ ] Replace `peek_cqe` with `wait_cqe_timeout`.
    - [ ] Implement `UringFile` (similar to `UringSocket`) to fix file API and shadow FD usage.
    - [ ] Improve separation between C and Python layers.
    - [ ] Move all Python imports to module scope

## Phase 2 — Core Features
- [ ] Fix Files API by implementing `UringFile` (analogous to `UringSocket`).
- [ ] Implement graceful shutdown.
- [ ] Translate internal errors into native Python exceptions.
- [ ] Proper GIL handling.
- [ ] Add timer support (`IORING_OP_TIMEOUT`).
- [ ] Add signal handling (`SIGINT`, `SIGTERM`) via `IORING_OP_POLL_ADD`.

## Phase 3 — Performance & io_uring Features
- [ ] Implement fixed buffers (`io_uring_register_buffers`).
- [ ] Implement zero-copy send (`send_zc`).
- [ ] Use Python application buffers as `UringLoop` buffers.
- [ ] Implement a puring-based memory pool for buffers.
- [ ] Implement provided buffer rings / multishot (`register_pbuf_ring`).
- [ ] CQE batching.
- [ ] Offload SQE submission to a kernel thread (`IORING_SETUP_SQPOLL`).
- [ ] Support linked SQEs for chained operations.
- [ ] Keep file descriptors registered (`io_uring_register_files`).

## Phase 4 — Async Runtime & Architecture
- [ ] Multiring support.
- [ ] Support all awaitable operations.
- [ ] Support modern Python async interfaces (`Runners`).
- [ ] Configurable ring parameters.
- [ ] Revisit loop replacement (currently uncertain but likely needed).
- [ ] Improve future creation (FutureFactory + future pool).

## Phase 5 — Quality & Distribution
- [ ] Write tests.
- [ ] Add CI pipeline.
- [ ] Publish library on PyPI (`pip` installable).
- [ ] Document everything.

## Phase 6 — Community & Contribution
- [ ] Add contribution support:
    - [ ] Write contribution guidelines.
    - [ ] Add repository badges.
    - [ ] Enable GitHub issue tracking.

---

### Note
This roadmap is expected to evolve as our understanding deepens and the project matures.
