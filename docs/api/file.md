# File

`class File` — aio_uring file adapter

## Members

| Member | Type | Access | Doc |
|---|---|---|---|
| `fd` | `int` | readonly | file descriptor |

## Methods

| Method | Args | Returns | Doc |
|---|---|---|---|
| `read` | `self, offset: int = -1, size: int = 1024, timeout_params: TimeoutParamsLike = ...` | `bytes` | Read file |
| `readv` | `self, buffers: Optional[Any] = None, offset: int = -1, nowait: bool = False, timeout_params: TimeoutParamsLike = ...` | `list[bytes]` | Read file, vectorized |
| `readv_raw` | `self, iovecs: Any, offset: int = 0, nowait: int = 0, timeout_params: TimeoutParamsLike = ...` | `int` | Read file, vectorized with custom iovecs |
| `write` | `self, data: bytes, offset: int = 0, timeout_params: TimeoutParamsLike = ...` | `int` | Write file |
| `writev` | `self, buffers: Sequence[bytes], flags: int = 0, offset: int = 0, timeout_params: TimeoutParamsLike = ...` | `int` | Write file, vectorized |
| `writev_raw` | `self, buffers: Any, flags: int = 0, offset: int = 0, timeout_params: TimeoutParamsLike = ...` | `int` | Write file, vectorized with custom iovecs |
| `close` | `self, timeout_params: TimeoutParamsLike = ...` | `None` | Close file |
| `fsync` | `self, timeout_params: TimeoutParamsLike = ...` | `None` | Flush file buffer to file |
| `fdatasync` | `self, timeout_params: TimeoutParamsLike = ...` | `None` | Flush file buffer to file with in fdatasync mode |
| `splice` | `self, src: int, dst: int, count: int, offset_src: int = 0, offset_dst: int = 0, flag: int = 0, timeout_params: TimeoutParamsLike = ...` | `int` | Splicing two file pipes |
| `__aenter__` | `self` | `File` | Entering async context manager |
| `__aexit__` | `self, exc_type: object, exc_val: object, exc_tb: object` | `Optional[bool]` | Closing async context manager |
