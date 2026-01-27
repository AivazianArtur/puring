void incapsulated(void) 
{
    io_uring_get_sqe();
    registry_add();
    io_uring_prep_read(); 
    io_uring_sqe_set_data();
    io_uring_submit();
}

static PyObject *
submit_request(
    UringLoop *loop,
    int opcode,
    int fd,
    PyObject *buffer,
    off_t offset,
    void *extra
)

{
    PyObject *future;
    struct request *req;
    struct io_uring_sqe *sqe;

    future = create_future(loop);

    req = request_create(opcode, fd, buffer, future);
    registry_add(loop->registry, req);

    sqe = io_uring_get_sqe(&loop->ring);

    opcode_router_prepare(sqe, req, offset, extra);

    sqe->user_data = req->id;

    io_uring_submit(&loop->ring);

    return future;
}
