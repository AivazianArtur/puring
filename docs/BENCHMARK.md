Puring is currently in Pre-Alpha. These benchmarks represent the 'floor' of its performance. We haven't even turned on the turbo yet


| Chunk Size | Chunk Size (KiB) | Standard (MB/s) | Asyncio Thread (MB/s) | Uvloop Thread (MB/s) | io_uring Seq (MB/s) |
|-----------:|-----------------:|----------------:|----------------------:|---------------------:|--------------------:|
| 1048576 | 1024 | 72.30 | 64.75 | 73.58 | 137.02 |
| 524288  | 512  | 65.38 | 56.00 | 61.89 | 128.94 |
| 262144  | 256  | 56.98 | 47.85 | 53.70 | 112.57 |
| 131072  | 128  | 46.69 | 41.96 | 49.11 | 101.51 |
| 65536   | 64   | 45.97 | 36.26 | 43.65 | 78.05 |
| 32768   | 32   | 46.60 | 32.54 | 43.00 | 66.72 |
| 16384   | 16   | 35.24 | 24.44 | 31.24 | 36.25 |
| 8192    | 8    | 25.36 | 17.07 | 21.55 | 18.37 |




## Performance Analysis

### Overview

The benchmark compares four write strategies across decreasing chunk sizes:

- synchronous writes (`standard`)
- `asyncio` + threadpool
- `uvloop` + threadpool
- `io_uring` sequential submission

### Key Findings

#### 1. io_uring dominates at large I/O sizes
For chunk sizes ≥128 KiB, `io_uring` achieves roughly **2× throughput** compared to traditional approaches.

Example:
- 1 MiB chunks:
    - standard: 72 MB/s
    - uvloop: 73 MB/s
    - io_uring: **137 MB/s**

This indicates reduced syscall overhead and better kernel-side batching.

#### 2. Threadpool async adds overhead
Both async threadpool approaches consistently underperform synchronous writes:

- extra context switching
- executor scheduling cost
- no true async disk I/O

`uvloop` slightly improves performance but cannot eliminate threadpool overhead.

#### 3. Performance scales with chunk size
All methods benefit from larger writes due to:

- fewer syscalls
- improved page cache efficiency
- reduced scheduler pressure

Throughput drops steadily below 64 KiB.

#### 4. Small I/O removes io_uring advantage
At 8–16 KiB:

- submission overhead becomes dominant
- syscall savings shrink
- performance converges with synchronous I/O

At 8 KiB, synchronous writes actually outperform `io_uring`.

### Practical Takeaways

- Use **io_uring** for:
    - large sequential writes
    - log streaming
    - file generation pipelines
    - storage engines

- Prefer **simple synchronous I/O** for:
    - small writes
    - latency-sensitive operations
    - low-throughput workloads

- `asyncio` threadpools are not a disk I/O optimization — they mainly help concurrency, not raw throughput.

### Rule of Thumb

| Chunk Size | Best Strategy |
|------------|--------------|
| ≥128 KiB | io_uring |
| 32–64 KiB | depends on workload |
| ≤16 KiB | synchronous I/O |

