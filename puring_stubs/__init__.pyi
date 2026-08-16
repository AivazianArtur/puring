"""Type stubs for the `puring` C extension (io_uring-backed asyncio loop)."""

from __future__ import annotations

import asyncio
import enum
from typing import Any, Awaitable, Optional, Sequence, Union

# --- Enums exported from create_*_enum() in execution_context.c -----------

class ResolveFlags(enum.IntFlag):
    """Flags for openat2 path resolution (see RESOLVE_* in linux/openat2.h)."""
    ...

class StatxFlags(enum.IntFlag):
    """Flags accepted by statx(2)."""
    ...

class StatxMask(enum.IntFlag):
    """Field mask accepted by statx(2)."""
    ...

class BUFFER_MODE(enum.IntEnum):
    """Buffer allocation strategy used for read operations."""
    ...

class STREAM_STRATEGY(enum.IntEnum):
    """Submission strategy: oneshot vs multishot/streamed reads."""
    ...

class TRANSFER_MODE(enum.IntEnum):
    """Data transfer mode (e.g. plain vs zero-copy)."""
    ...

class PAYLOAD_TYPE(enum.IntEnum):
    """Internal payload classification used by the registry."""
    ...


# --- Timeout params ---------------------------------------------------------
# Passed as `timeout_params=` to most I/O methods; treat as opaque mapping/tuple
# unless you know the exact shape parse_timeout_params() expects.
TimeoutParamsLike = Any


# --- Context managers returned by PuringLoop.buffer_mode()/stream_strategy()/
#     transfer_mode()/execution_context() -----------------------------------

class BufferModeCtx:
    def __enter__(self) -> "BufferModeCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...

class StreamStrategyCtx:
    def __enter__(self) -> "StreamStrategyCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...

class TransferModeCtx:
    def __enter__(self) -> "TransferModeCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...

class ExecutionContextCtx:
    def __enter__(self) -> "ExecutionContextCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...


# --- File -------------------------------------------------------------------

class File:
    """Puring file adapter returned by :func:`puring.open_file`."""

    fd: int  # readonly

    async def read(
        self, offset: int = -1, size: int = 1024, timeout_params: TimeoutParamsLike = ...
    ) -> bytes: ...
    async def readv(
        self,
        buffers: Optional[Any] = None,
        offset: int = -1,
        nowait: bool = False,
        timeout_params: TimeoutParamsLike = ...,
    ) -> list[bytes]: ...
    async def readv_raw(
        self,
        iovecs: Any,
        offset: int = 0,
        nowait: int = 0,
        timeout_params: TimeoutParamsLike = ...,
    ) -> int: ...
    async def write(
        self, data: bytes, offset: int = 0, timeout_params: TimeoutParamsLike = ...
    ) -> int: ...
    async def writev(
        self,
        buffers: Sequence[bytes],
        flags: int = 0,
        offset: int = 0,
        timeout_params: TimeoutParamsLike = ...,
    ) -> int: ...
    async def writev_raw(
        self,
        buffers: Any,
        flags: int = 0,
        offset: int = 0,
        timeout_params: TimeoutParamsLike = ...,
    ) -> int: ...
    async def close(self, timeout_params: TimeoutParamsLike = ...) -> None: ...
    async def fsync(self, timeout_params: TimeoutParamsLike = ...) -> None: ...
    async def fdatasync(self, timeout_params: TimeoutParamsLike = ...) -> None: ...
    async def splice(
        self,
        src: int,
        dst: int,
        count: int,
        offset_src: int = 0,
        offset_dst: int = 0,
        flag: int = 0,
        timeout_params: TimeoutParamsLike = ...,
    ) -> int: ...

    async def __aenter__(self) -> "File": ...
    async def __aexit__(
        self, exc_type: object, exc_val: object, exc_tb: object
    ) -> Optional[bool]: ...


# --- Socket -------------------------------------------------------------------

class Socket:
    """Puring socket adapter returned by :func:`puring.prep_socket`."""

    async def bind(
        self, host: str, port: int, timeout_params: TimeoutParamsLike = ...
    ) -> Any: ...
    async def connect(
        self, host: str, port: int, timeout_params: TimeoutParamsLike = ...
    ) -> Any: ...
    async def listen(self, backlog: int = 1, timeout_params: TimeoutParamsLike = ...) -> Any: ...
    async def accept(self, timeout_params: TimeoutParamsLike = ...) -> Any: ...
    async def close(self, timeout_params: TimeoutParamsLike = ...) -> None: ...
    async def send(
        self, data: bytes, is_poll_first: bool = False, timeout_params: TimeoutParamsLike = ...
    ) -> int: ...
    async def recv(
        self,
        bufsize: int = 1024,
        buffer: Optional[Any] = None,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsLike = ...,
    ) -> bytes: ...
    async def sendto(
        self,
        data: bytes,
        host: str,
        port: int,
        domain: int,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsLike = ...,
    ) -> int: ...
    async def recvfrom(
        self,
        bufsize: int = 1024,
        buffer: Optional[Any] = None,
        host: str = ...,
        port: int = ...,
        domain: int = ...,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsLike = ...,
    ) -> Any: ...
    async def sendmsg(
        self,
        buffers: Any,
        host: Optional[str] = None,
        port: int = 0,
        domain: int = 0,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsLike = ...,
    ) -> int: ...
    async def recvmsg(
        self,
        buffers: Optional[Any] = None,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsLike = ...,
    ) -> Any: ...

    async def __aenter__(self) -> "Socket": ...
    async def __aexit__(
        self, exc_type: object, exc_val: object, exc_tb: object
    ) -> Optional[bool]: ...


# --- PuringLoop ---------------------------------------------------------------

class PuringLoop(asyncio.BaseEventLoop):
    """asyncio-compatible event loop backed by io_uring."""

    def __init__(self, registry_size: int = 0) -> None: ...

    def close(self) -> None: ...

    def buffer_mode(
        self,
        mode: BUFFER_MODE = ...,
        buffers: Optional[Any] = None,
        payload_type: PAYLOAD_TYPE = ...,
        amount: int = 3,
        bufsize: int = 1024,
    ) -> BufferModeCtx: ...
    def stream_strategy(self, stream: STREAM_STRATEGY = ...) -> StreamStrategyCtx: ...
    def transfer_mode(self, mode: TRANSFER_MODE = ...) -> TransferModeCtx: ...
    def execution_context(
        self,
        buffer_mode: BUFFER_MODE = ...,
        stream_strategy: STREAM_STRATEGY = ...,
        transfer_mode: TRANSFER_MODE = ...,
        buffers: Optional[Any] = None,
        payload_type: PAYLOAD_TYPE = ...,
        amount: int = 3,
        bufsize: int = 1024,
    ) -> ExecutionContextCtx: ...

    def _run_once(self) -> None: ...
    def _write_to_self(self) -> None: ...


# --- Module-level functions ---------------------------------------------------

def timer(
    uring_loop: PuringLoop,
    timer_params: Optional[Any] = None,  # object with int fields: sec, nsec, count
) -> Awaitable[None]:
    """Sets a timer."""
    ...

def open_file(
    path: Union[str, "os.PathLike[str]"],
    dirfd: int = ...,
    flags: int = -1,
    resolve: int = 0,
    mode: int = 0o644,
    timeout_params: TimeoutParamsLike = ...,
) -> Awaitable[File]:
    """Opens a file and instantiates a :class:`File` object."""
    ...

def prep_socket(
    domain: int = ...,  # socket.AF_INET
    socktype: int = ...,  # socket.SOCK_STREAM
    timeout_params: TimeoutParamsLike = ...,
) -> Awaitable[Socket]:
    """Opens a socket and instantiates a :class:`Socket` object."""
    ...
