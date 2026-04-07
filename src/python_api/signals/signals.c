#include "signals.h"


int set_signals_handler(struct io_uring *ring) {
    // TODO: When we'll be doing multishot, have to multishot this
    int pipefd[2];
    pipe2(pipefd, O_NONBLOCK);
    PySignal_SetWakeupFd(pipefd[1]);
    
    struct io_uring_sqe *sqe = create_sqe(ring);
    io_uring_prep_poll_add(sqe, pipefd[0], POLLIN);
    
    SignalsData *signals_data = malloc(sizeof(SignalsData));
    signals_data->fd = pipefd[0];

    io_uring_sqe_set_data(sqe, signals_data);
    return 1;
}
