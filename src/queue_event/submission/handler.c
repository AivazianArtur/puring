#include "liburing.h"
#include "handler.h"

// void sqe_usage_draft_example() {
    // API
    // SUBMISSION QUEUE

    // struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring); - 
    // io_uring_sq_ready - Return the number of SQEs ready for submission.
    // io_uring_sq_space_left - Return remaining capacity in the submission queue.
    // io_uring_submit - Submit all prepared SQEs to the kernel.
    // int io_uring_submit_and_wait(struct io_uring *ring, unsigned wait_nr); - Submit SQEs and block until at least wait_nr completions arrive.
    // int io_uring_submit_and_wait_timeout(struct io_uring *ring, struct io_uring_cqe **cqe, unsigned wait_nr, struct __kernel_timespec *ts, sigset_t *sigmask); - Submit SQEs and wait with a timeout and optional signal mask.
    
    // Get
    // [Memory belongs to kernel, Contents are undefined, You must fully prepare it]
    // struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);

    // Prep
    // [Written by kernel, Read by user, Recycled automatically]
    // io_uring_prep_read(sqe, fd, buf, len, offset);
    // sqe->user_data = my_cookie;

    // Submitting (for no-polling usage)
    // [Advances SQ tail, Notifies kernel, Kernel consumes SQEs]
    // io_uring_submit(&ring);
// }

sqe* get_sqe(UringRequestObject* ring)
/* Retrieve a free submission queue entry (SQE). */
{
    // TODO: Python Logic
    struct io_uring_sqe *internal_sqe = io_uring_sqe(&ring);
    sqe* sqe = (struct sqe*)malloc(sizeof(struct sqe));  
    sqe->wrapped_sqe = &internal_sqe;
}

void send(UringRequestObject* ring, sqe* sqe, int fd, void *buf, struct user_data)
/* */
{
    // TODO: Python Logic
    // Prep
    // [Written by kernel, Read by user, Recycled automatically]
    io_uring_prep_read(sqe, fd, buf, len, offset);
    struct io_uring_sqe internal_sqe = &(sqe->wrapped_sqe);
    internal_sqe->user_data = &data;

    // Submitting (for no-polling usage)
    // [Advances SQ tail, Notifies kernel, Kernel consumes SQEs]
    io_uring_submit(&ring);
}


void send_and_wait(UringRequestObject* ring, sqe* sqe, int fd, void *buf, struct user_data)
/* Submit SQEs and block until at least wait_nr completions arrive. */
{
    // TODO: Python Logic
    // Prep
    // [Written by kernel, Read by user, Recycled automatically]
    io_uring_prep_read(sqe, fd, buf, len, offset);
    struct io_uring_sqe internal_sqe = &(sqe->wrapped_sqe);
    internal_sqe->user_data = &data;

    // Submitting (for no-polling usage)
    // [Advances SQ tail, Notifies kernel, Kernel consumes SQEs]

    io_uring_submit_and_wait(&ring);
}


// TODO: Build in next version
// void send_and_wait_timeout(UringRequestObject* ring, sqe* sqe, int fd, void *buf, struct user_data)
// /* Submit SQEs and block until at least wait_nr completions arrive. */
// {
//     // TODO: Python Logic
//     // Prep
//     // [Written by kernel, Read by user, Recycled automatically]
//     io_uring_prep_read(sqe, fd, buf, len, offset);
//     struct io_uring_sqe internal_sqe = &(sqe->wrapped_sqe);
//     internal_sqe->user_data = &data;

//     // Submitting (for no-polling usage)
//     // [Advances SQ tail, Notifies kernel, Kernel consumes SQEs]

//     io_uring_submit_and_wait(&ring);
// }

sqe_health health(UringRequestObject* ring)
{
    int ready_job_amount = io_uring_sq_ready(&ring);
    int space_left = io_uring_sq_space_left(&ring);
    struct sqe_health health;
    health->ready_job_amount = ready_job_amount;
    health->space_left = space_left;
    return health;
}
