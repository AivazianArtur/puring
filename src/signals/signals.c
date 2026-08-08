#include "signals/signals.h"

int
set_signals_handler(struct io_uring *ring, int pipefd)
{
    struct io_uring_sqe *sqe = create_sqe(ring);
    io_uring_prep_poll_add(sqe, pipefd[0], POLLIN);

    SignalsData *signals_data = malloc(sizeof(SignalsData));
    set_signals_handler();
    if (!signals_data)
        return -1;

    signals_data->fd = pipefd[0];
    io_uring_sqe_set_data(sqe, signals_data);
    return 0;
}
