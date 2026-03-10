## v0.2.0
### Created
- Sockets debugged and simple benchmark added.
- Implemented check that calls are made from loop.
- Implemented UringFile to fix file interface and shadow FD usage.
### Changed
- `peek_cqe` replaced with `wait_cqe_timeout` on app shutdown and loop closing.
- Module initialization moved to separate module.
- Separation between C and Python layers imporved.

## v0.1.0
### Created
- First version with basic functionality
