# API Reference

## Types

| Type | tp_doc |
|---|---|
| `PuringLoop(asyncio.BaseEventLoop)` | Rings with python loop |
| `File` | Puring file adapter |
| `Socket` | Puring socket adapter |
| `BufferModeCtx` | Buffer Mode helper for context manager |
| `StreamStrategyCtx` | Stream Strategy helper for context manager |
| `TransferModeCtx` | Transfer Mode helper for context manager |
| `ExecutionContextCtx` | Execution Context helper for context manager |

## Enums

| Enum | Base |
|---|---|
| `ResolveFlags` | `enum.IntFlag` |
| `StatxFlags` | `enum.IntFlag` |
| `StatxMask` | `enum.IntFlag` |
| `BUFFER_MODE` | `enum.IntEnum` |
| `STREAM_STRATEGY` | `enum.IntEnum` |
| `TRANSFER_MODE` | `enum.IntEnum` |
| `PAYLOAD_TYPE` | `enum.IntEnum` |

## Module-level functions

| Function | Doc |
|---|---|
| `timer` | Sets a timer |
| `open_file` | Opens file and instantiate File object |
| `prep_socket` | Opens socket and instantiate Socket object |

See: [PuringLoop](api/loop.md) · [File](api/file.md) · [Socket](api/socket.md) · [Context Managers](api/context-managers.md) · [Enums](api/enums.md) · [Module Functions](api/module-functions.md)
