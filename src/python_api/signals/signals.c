#include "signals/signals.h"

int
set_signals_controller(struct io_uring *ring) {
    int pipefd[2];
    pipe2(pipefd, O_NONBLOCK);
    PySignal_SetWakeupFd(pipefd[1]);
    int result = set_signals_handler(ring, pipefd);
    if (result < 0) {
        PyErr_NoMemory();
        return 0;
    }
    return 1;
}
