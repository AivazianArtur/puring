# io_uring
# io_uring is an efficient Linux implementation of the Proactor I/O model.
It provides two shared buffers:
* Submission Queue - Where apps pushes I/O requests.
* Completion Queue - Where kernel pushes I/O responses.
#### The core idea is that applications do not need to wait for readiness events. Instead of polling or blocking, an application submits operations to the Submission Queue and later reads exact completion results from the Completion Queue. Rather than receiving “socket is ready,” the application receives the actual result of the I/O operation.

### To understand how uring works, let`s see how standart reactor pattern works by zooming at epoll
## Epoll

![Epoll diagramm](/docs/uring/images/epoll.png)

Only when epoll_wait(`WAIT` on diagram) returns result, app can do real I/O (read/write from receive buffer)