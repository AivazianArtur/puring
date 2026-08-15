# Context Managers

All four share the same method shape (`__enter__`/`__exit__`).

| Type | tp_doc |
|---|---|
| `BufferModeCtx` | Buffer Mode helper for context manager |
| `StreamStrategyCtx` | Stream Strategy helper for context manager |
| `TransferModeCtx` | Transfer Mode helper for context manager |
| `ExecutionContextCtx` | Execution Context helper for context manager |

## Methods

| Method | Args | Returns | Doc |
|---|---|---|---|
| `__enter__` | `self` | `Self` | Entering context manager |
| `__exit__` | `self, exc_type: object, exc_val: object, exc_tb: object` | `Optional[bool]` | Closing context manager |
