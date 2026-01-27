#include "core.h"


int ring_init(memory_params *memory_params, ring_init_params *params)
{
    // if (memory_params) {
    //     if (!params) {
    //         params = {0}
    //     }
    //     result = io_uring_queue_init_mem(),
    // }
    // else if (params) {
    //     // TODO
    //     result = io_uring_queue_init_params(8, ring, params);
    // }
    // else {

    // Now only default initialization
    // TODO: Do full initialization
    result = io_uring_queue_init(0, ring, 0);
    return 0;
}

void ring_destroy(io_uring* ring) 
{
    io_uring_queue_exit(ring);
}
