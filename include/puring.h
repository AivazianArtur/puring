#pragma once  // Alternative of `include guards (IFNDEF)`
#include "liburing.h"

int puring_init(struct io_uring *ring);
void puring_exit(struct io_uring *ring);
