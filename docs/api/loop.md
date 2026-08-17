# AioUringLoop

`class AioUringLoop(asyncio.BaseEventLoop)` — Rings with python loop

```python
__init__(self, registry_size: int = 0) -> None
```

| Method | Args | Returns | Doc |
|---|---|---|---|
| `close` | `self` | `None` | Close loop |
| `buffer_mode` | `self, mode: BUFFER_MODE = ..., buffers: Optional[Any] = None, payload_type: PAYLOAD_TYPE = ..., amount: int = 3, bufsize: int = 1024` | `BufferModeCtx` | Init context to run loop with specific buffer mode |
| `stream_strategy` | `self, stream: STREAM_STRATEGY = ...` | `StreamStrategyCtx` | Init context to run loop with specific stream strategy |
| `transfer_mode` | `self, mode: TRANSFER_MODE = ...` | `TransferModeCtx` | Init context to run loop with specific transfer mode |
| `execution_context` | `self, buffer_mode: BUFFER_MODE = ..., stream_strategy: STREAM_STRATEGY = ..., transfer_mode: TRANSFER_MODE = ..., buffers: Optional[Any] = None, payload_type: PAYLOAD_TYPE = ..., amount: int = 3, bufsize: int = 1024` | `ExecutionContextCtx` | Init context to run loop with specific execution context settings |
| `_run_once` | `self` | `None` | Run one full iteration of the event loop |
| `_write_to_self` | `self` | `None` | Write a byte to self-pipe, to wake up the event loop |
