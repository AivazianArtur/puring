## v0.4.0
### Created
- Implemented new layer - Buffer Layer:
    - Main struct - `BufferPayload`
    - Distinguished Linear and Vectored buffer types
    - Interface for different scenarios of buffer creation
- Support of advanced io_uring features via new layer - `ExecuitionContext`: 
  - Support of fixed buffers, including implementation of registry for fixed buffers
  - Support of provided buffers
  - Partial support of buffer ring
  - Support of zero-copy operations, via 
  - Partial support of multishot operations 
  - Special ContextVar to keep `ExecutionContext`
  - Special context managers: `ProvidedBuffers`, `StreamStrategy`, `TransferMode`, `ExecutionContext`
- Support of context manager protocol for `File` and `Socket`
- Initial documentation: architecture.md, developing.md, user_guide.md, contributing.md, io_uring.md
- Initial Python and C tests (AI generated)
- Add benchmarks (AI generated)
### Updated
- Makefile: fixed sanitaizer stage, added new stages.
- GitHub CI: partly added tests stage
- Bunch of memory related bug fixes
### Deleted
- GitHub CI: removed sanitizer stage(redundant)

## v0.3.0
### Created
- Raising Python exceptions.
- Handling of system signals.
- Implemented event's timer interface. 
- Prepared interface for event timeout.
- Implemented GitHUb CI with linter stages.
### Updated
- `Uring` prefix renamed to `Puring`.
- `PuringLoop` became child of asyncio event-loop.
- `PuringFile` interface completed.
- `PuringSocket` interface completed.
- Makefile includes linter stages.

## v0.2.0
### Created
- Sockets debugged and simple benchmark added.
- Implemented check that calls are made from loop.
- Implemented UringFile to fix file interface and shadow FD usage.
### Updated
- `peek_cqe` replaced with `wait_cqe_timeout` on app shutdown and loop closing.
- Module initialization moved to separate module.
- Separation between C and Python layers imporved.

## v0.1.0
### Created
- First version with basic functionality
