"""Type stubs for the `uringio` C extension (io_uring-backed asyncio loop)."""

from __future__ import annotations

import os
import asyncio
import enum
from typing import Any, Awaitable, Optional, Sequence, TypedDict, Union, final
from typing_extensions import disjoint_base

# --- Enums exported from create_*_enum() in execution_context.c and ops/files/enums.c-----------

class ResolveFlags(enum.IntFlag):
    """Flags for openat2 path resolution (see RESOLVE_* in linux/openat2.h)."""
    NO_XDEV = 1
    NO_MAGICLINKS = 2
    NO_SYMLINKS = 4
    BENEATH = 8
    IN_ROOT = 16
    CACHED = 32

class StatxFlags(enum.IntFlag):
    """Flags accepted by statx(2)."""
    EMPTY_PATH = 0x1000
    SYMLINK_NOFOLLOW = 0x100
    NO_AUTOMOUNT = 0x800
    STATX_SYNC_AS_STAT = 0x0000
    STATX_FORCE_SYNC = 0x2000
    STATX_DONT_SYNC = 0x4000

class StatxMask(enum.IntFlag):
    """Field mask accepted by statx(2)."""
    TYPE = 0x0001
    MODE = 0x0002
    NLINK = 0x0004
    UID = 0x0008
    GID = 0x0010
    ATIME = 0x0020
    MTIME = 0x0040
    CTIME = 0x0080
    INO = 0x0100
    SIZE = 0x0200
    BLOCKS = 0x0400
    BASIC_STATS = 0x07ff
    BTIME = 0x0800
    ALL = 0x0fff

class BUFFER_MODE(enum.IntEnum):
    """Buffer allocation strategy used for read operations."""
    NORMAL = 0
    FIXED = 1
    PROVIDED = 2
    BUF_RING = 3

class STREAM_STRATEGY(enum.IntEnum):
    """Submission strategy: oneshot vs multishot/streamed reads."""
    ONESHOT = 0
    MULTISHOT = 1

class TRANSFER_MODE(enum.IntEnum):
    """Data transfer mode (e.g. plain vs zero-copy)."""
    NORMAL = 0
    ZERO_COPY = 1

class PAYLOAD_TYPE(enum.IntEnum):
    """Internal payload classification used by the registry."""
    LINEAR = 0
    IOVEC = 1
    LINEAR_AND_IOVEC = 2

# --- Timeout params ---------------------------------------------------------
class TimeoutParamsDict(TypedDict, total=False):
    sec: int
    nsec: int
    is_required: bool


# --- Context managers returned by UringioLoop.buffer_mode()/stream_strategy()/
#     transfer_mode()/execution_context() -----------------------------------

@final
class BufferModeCtx:
    def __enter__(self) -> "BufferModeCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...

@final
class StreamStrategyCtx:
    def __enter__(self) -> "StreamStrategyCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...

@final
class TransferModeCtx:
    def __enter__(self) -> "TransferModeCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...

@final
class ExecutionContextCtx:
    def __enter__(self) -> "ExecutionContextCtx": ...
    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> Optional[bool]: ...


# --- File -------------------------------------------------------------------

@final
class File:
    """uringio file adapter returned by :func:`uringio.open_file`."""

    fd: int  # readonly

    async def read(
        self, offset: int = -1, size: int = 1024, timeout_params: TimeoutParamsDict = ...
    ) -> bytes: ...
    async def readv(
        self,
        buffers: Optional[Any] = None,
        offset: int = -1,
        nowait: bool = False,
        timeout_params: TimeoutParamsDict = ...,
    ) -> list[bytes]: ...
    async def readv_raw(
        self,
        iovecs: Any,
        offset: int = 0,
        nowait: int = 0,
        timeout_params: TimeoutParamsDict = ...,
    ) -> int: ...
    async def write(
        self, data: bytes, offset: int = 0, timeout_params: TimeoutParamsDict = ...
    ) -> int: ...
    async def writev(
        self,
        buffers: Sequence[bytes],
        flags: int = 0,
        offset: int = 0,
        timeout_params: TimeoutParamsDict = ...,
    ) -> int: ...
    async def writev_raw(
        self,
        buffers: Any,
        flags: int = 0,
        offset: int = 0,
        timeout_params: TimeoutParamsDict = ...,
    ) -> int: ...
    async def close(self, timeout_params: TimeoutParamsDict = ...) -> None: ...
    async def fsync(self, timeout_params: TimeoutParamsDict = ...) -> None: ...
    async def fdatasync(self, timeout_params: TimeoutParamsDict = ...) -> None: ...
    async def splice(
        self,
        src: int,
        dst: int,
        count: int,
        offset_src: int = 0,
        offset_dst: int = 0,
        flag: int = 0,
        timeout_params: TimeoutParamsDict = ...,
    ) -> int: ...

    async def __aenter__(self) -> "File": ...
    async def __aexit__(
        self, exc_type: object, exc_val: object, exc_tb: object
    ) -> Optional[bool]: ...


# --- Socket -------------------------------------------------------------------

@final
class Socket:
    """uringio socket adapter returned by :func:`uringio.prep_socket`."""

    async def bind(
        self, host: str, port: int, timeout_params: TimeoutParamsDict = ...
    ) -> Any: ...
    async def connect(
        self, host: str, port: int, timeout_params: TimeoutParamsDict = ...
    ) -> Any: ...
    async def listen(self, backlog: int = 1, timeout_params: TimeoutParamsDict = ...) -> Any: ...
    async def accept(self, timeout_params: TimeoutParamsDict = ...) -> Any: ...
    async def close(self, timeout_params: TimeoutParamsDict = ...) -> None: ...
    async def send(
        self, data: bytes, is_poll_first: bool = False, timeout_params: TimeoutParamsDict = ...
    ) -> int: ...
    async def recv(
        self,
        bufsize: int = 1024,
        buffer: Optional[Any] = None,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsDict = ...,
    ) -> bytes: ...
    async def sendto(
        self,
        data: bytes,
        host: str,
        port: int,
        domain: int,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsDict = ...,
    ) -> int: ...
    async def recvfrom(
        self,
        bufsize: int = 1024,
        buffer: Optional[Any] = None,
        host: str = ...,
        port: int = ...,
        domain: int = ...,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsDict = ...,
    ) -> Any: ...
    async def sendmsg(
        self,
        buffers: Any,
        host: Optional[str] = None,
        port: int = 0,
        domain: int = 0,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsDict = ...,
    ) -> int: ...
    async def recvmsg(
        self,
        buffers: Optional[Any] = None,
        is_poll_first: bool = False,
        timeout_params: TimeoutParamsDict = ...,
    ) -> Any: ...

    async def __aenter__(self) -> "Socket": ...
    async def __aexit__(
        self, exc_type: object, exc_val: object, exc_tb: object
    ) -> Optional[bool]: ...


# --- UringioLoop ---------------------------------------------------------------

@disjoint_base
class UringioLoop(asyncio.BaseEventLoop):
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
class TimerParamsDict(TypedDict, total=False):
    sec: int
    nsec: int
    count: int
    is_multishot: bool


def timer(
    uring_loop: UringioLoop,
    timer_params: Optional[TimerParamsDict] = None,
) -> Awaitable[None]:
    """Sets a timer."""
    ...

def open_file(
    path: Union[str, "os.PathLike[str]"],
    dirfd: int = ...,
    flags: int = -1,
    resolve: int = 0,
    mode: int = 0o644,
    timeout_params: TimeoutParamsDict = ...,
) -> Awaitable[File]:
    """Opens a file and instantiates a :class:`File` object."""
    ...

def prep_socket(
    domain: int = ...,  # socket.AF_INET
    socktype: int = ...,  # socket.SOCK_STREAM
    timeout_params: TimeoutParamsDict = ...,
) -> Awaitable[Socket]:
    """Opens a socket and instantiates a :class:`Socket` object."""
    ...
