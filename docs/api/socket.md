# Socket

`class Socket` — uringio socket adapter

## Methods

| Method | Args | Returns | Doc |
|---|---|---|---|
| `bind` | `self, host: str, port: int, timeout_params: TimeoutParamsLike = ...` | `Any` | Bind socket |
| `connect` | `self, host: str, port: int, timeout_params: TimeoutParamsLike = ...` | `Any` | Connect |
| `listen` | `self, backlog: int = 1, timeout_params: TimeoutParamsLike = ...` | `Any` | Listen socket |
| `accept` | `self, timeout_params: TimeoutParamsLike = ...` | `Any` | Accept |
| `close` | `self, timeout_params: TimeoutParamsLike = ...` | `None` | Close |
| `send` | `self, data: bytes, is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...` | `int` | Send |
| `recv` | `self, bufsize: int = 1024, buffer: Optional[Any] = None, is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...` | `bytes` | Recv |
| `sendto` | `self, data: bytes, host: str, port: int, domain: int, is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...` | `int` | Sendto |
| `recvfrom` | `self, bufsize: int = 1024, buffer: Optional[Any] = None, host: str = ..., port: int = ..., domain: int = ..., is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...` | `Any` | Recvfrom |
| `sendmsg` | `self, buffers: Any, host: Optional[str] = None, port: int = 0, domain: int = 0, is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...` | `int` | Sendmsg |
| `recvmsg` | `self, buffers: Optional[Any] = None, is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...` | `Any` | Recvmsg |
| `__aenter__` | `self` | `Socket` | Entering async context manager |
| `__aexit__` | `self, exc_type: object, exc_val: object, exc_tb: object` | `Optional[bool]` | Closing async context manager |
