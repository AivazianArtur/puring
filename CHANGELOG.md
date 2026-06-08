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
