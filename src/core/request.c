void incapsulated(void) 
{
    io_uring_get_sqe();
    registry_add();
    io_uring_prep_read(); 
    io_uring_sqe_set_data();
    io_uring_submit();
}