# io_uring

## io_uring is an efficient Linux implementation of the Proactor I/O model.
### It provides two shared buffers:
* Submission Queue - Where apps pushes I/O requests.
* Completion Queue - Where kernel pushes I/O responses.

### The core difference between this and standard reactor model using epoll is that proactor models provides solutions to reduce system calls for getting result. 
Let`s look at this difference by looking at diagrams of two phases:
1. Sending to kernel
2. Getting result


## Sending to Kernel 
### Epoll
![Epoll diagramm](/docs/uring/images/epoll_send_to_kernel.png)
1. First, you`re creating socket and getting fd.
2. By passing fd, socket is registered in epoll's interest list + in socket's `Wait Queue` is passing callback.
3. Then user is making I/O operation. At this point, all user's information is in `User Buffer`.
4. Syscall is coming to kernel through CPU.
5. Kernel copies from `User Buffer` to socket's `Send Buffer`.
6. Kernel is executing command, for example - send to `Network Controller` (abstraction).
7. When controller returns to kernel, it saves result to socket's `Receive Buffer`.

### Uring
![Uring diagramm](/docs/uring/images/uring_send_to_kernel.png)
1. First, you`re creating socket and getting fd.
2. Then user is making I/O operation. At this point, all user's information is in `User Buffer`.
3. Before making syscall, uring client is creating `SQE`.
4. Then syscall is making - straight to `Submission Queue`.
5. Rings are structures, but uring also have helpers. Here let's call all of them just `uring_executor`.
6. `uring_executor` is creating `io_kiocb` object. This is complex object, that provides functionality to kernel create retry callback and return callback.
7. Kernel copies from `User Buffer` to socket's `Send Buffer`.
8. Kernel is executing command, for example - send to `Network Controller` (abstraction).
9. When controller returns to kernel, it saves result to socket's `Receive Buffer`.
10. Most important part: kernel writes data to `User Buffer`. Asynchronously!

## Getting Result 
### Epoll
![Epoll diagramm](/docs/uring/images/epoll_get_result.png)
1. At this point, state of socket is changing and callback inside `Wait Queue` are calling.
2. This callback makes two things:
   1. Fills ready list with result
   2. Calls `wake up`.
3. Component that waits for result, for us it is `loop`, is says to app - `you can get result`
4. App is making call to kernel.
5. Kernel gets content of `Receive Buffer`.
6. Kernel copies result to `User Buffer`.

### Uring
![Uring diagramm](/docs/uring/images/uring_get_result.png)
1. After writing to `User Buffer`, kernel destroys `io_kiocb`.
2. Kernel creates CQE and sends it straight to `Completion Queue`.
3. Kernel calls `wake up` to app.
4. Loop is getting this and says to app - `you can get result`.
5. Data is already in user buffer - use it


Those diagrams are showing the simplest configurations of both `epoll` and `uring`.
Both system can be more optimized by implement alternative for `wake up`, buffer optimizations, setting retires and more from the box.


## Quick Summarize
| Feature        | Epoll (Reactor)                         | io_uring (Proactor)                         |
|---------------|------------------------------------------|---------------------------------------------|
| Notification  | "It's ready, you do it."                | "It's done, here is the result."            |
| Syscalls      | 2+ (Wait + Read/Write)                  | 1 (Submission)                              |
| Data Copy     | Synchronous (blocks CPU)                | Asynchronous (Kernel handles it)            |
| Thread Usage  | Often needs a thread pool for files     | Truly single-threaded async                 |
