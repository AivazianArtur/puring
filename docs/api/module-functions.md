# Module Functions

| Function | Args | Returns | Doc |
|---|---|---|---|
| `timer` | `uring_loop: UringioLoop, timer_params: Optional[Any] = None` | `Awaitable[None]` | Sets a timer |
| `open_file` | `path: Union[str, os.PathLike[str]], dirfd: int = ..., flags: int = -1, resolve: int = 0, mode: int = 0o644, timeout_params: TimeoutParamsLike = ...` | `Awaitable[File]` | Opens file and instantiate File object |
| `prep_socket` | `domain: int = ..., socktype: int = ..., timeout_params: TimeoutParamsLike = ...` | `Awaitable[Socket]` | Opens socket and instantiate Socket object |
