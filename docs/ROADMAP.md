# Roadmap
* [ ]
* [ ] Support
* [ ]  Polish current implementation:
    * [ ] Move initialization of Python objects to separate module.
    * [ ] Debug, add simple benchmark and write examples for socket api.
    * [ ] Write ASSERT_LOOP_THREAD macros.
    * [ ] `peek_cqe` -> `wait_cqe_timeout`.
    * [ ] For fixing file api and shadow fd usage, implement UringFile, like UringSocket.
    * [ ] Write Makefile to import liburing and install deps.
    * [ ] Better separation of C and Python layers
* [ ] Fix files API - implement UringFile, like UringSocket.
* [ ]  Write tests.
* [ ] Add CI pipeline.
* [ ] Make library available through pip.
* [ ] Grace shutdown.
* [ ] Implement uring fixed buffers(`io_uring_register_buffers`).
* [ ] Implement uring zero-copy usage(`send_zc`). 
* [ ] Usage of python app buffer as UringLoop buffer.
* [ ] Puring based memory pool for buffers.
* [ ] Implement provided buffer ring aka `Multishot` (`register_pbuf_ring`).
* [ ] Translating errors to native Python errors.
* [ ] Catching signals, like `SIGINT` or `SIGTERM` (`IORING_OP_POLL_ADD `).
* [ ] Timer support (`IORING_OP_TIMEOUT`).
* [ ] Proper work with GIL.
* [ ] Multiring support.
* [ ] Support every awaitable operation.
* [ ] Support modern python async interface - `Runners`.
* [ ] Contribution support:kj
    * [ ] Write contribution guideline.
    * [ ] Add badges.
    * [ ] Add issue-system in github.
* [ ] Parameters to configure rings.
* [ ] Replace loop (maybe no need for this, but from this point looks like we need it).
* [ ] CQE batching.
* [ ] Offload SQE submission to a kernel thread(`IORING_SETUP_SQPOLL`).
* [ ] Link QE`s to chain operations.
* [ ] Keep file descriptors open - `io_uring_register_files`.
* [ ] Better future creation - FutureFactory and future pool

### Time is going, our understanding will change  with it, and this list will have updates.
