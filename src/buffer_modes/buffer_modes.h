#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/openat2.h>
#include <netinet/in.h>

#include "liburing.h"

#include "macroses.h"


int
init_fixed_mode(struct io_uring *ring, struct iovec *iovecs, unsigned nr_iovecs);

int
close_fixed_mode(struct io_uring *ring);

int
init_provided_mode(struct io_uring *ring, void *addr, int len, int nr, int bgid);

int
close_provided_mode(struct io_uring *ring, int nr, int bgid);

int
init_buf_ring_mode(struct io_uring *ring, struct io_uring_buf_reg *reg, unsigned int flags);

int
close_buf_ring_mode(struct io_uring *ring, struct io_uring_buf_reg *reg);
