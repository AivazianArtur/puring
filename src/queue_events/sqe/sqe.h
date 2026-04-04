#pragma once

#include <stdio.h>

#include "liburing.h"


struct io_uring_sqe* create_sqe(struct io_uring *ring);
