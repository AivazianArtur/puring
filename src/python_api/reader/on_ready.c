#include "reader.h"

void
on_uring_ready(PuringLoop *self) {
    struct io_uring_cqe *cqe;

    while (io_uring_peek_cqe(self->ring, &cqe) == 0) {
        int index = (int)(uintptr_t)cqe->user_data;
        RequestSlot *slot = registry_get(self->registry, index);

        if (!slot || !slot->future) {
            io_uring_cqe_seen(self->ring, cqe);
            continue;
        }

        PyObject *result = NULL;
        PyObject *exc = NULL;
        VectoredBuffer *vec;
        Py_ssize_t remaining;

        bool timeout_expired = (slot->opcode == IORING_OP_TIMEOUT && cqe->res == -ETIME);

        if (cqe->res < 0 && !timeout_expired) {
            exc = PyObject_CallFunction(PyExc_OSError, "i", -cqe->res);
            if (exc) {
                PyObject_CallMethod(slot->future, "set_exception", "O", exc);
                Py_DECREF(exc);
            } else {
                PyErr_Print();
            }
        } else {
            if (IS_SIGNALS_DATA(cqe->user_data)) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
                char buf[64];
#pragma clang diagnostic pop

                struct SignalsData *signals_data = (struct SignalsData *)cqe->user_data;
                read(signals_data->fd, buf, sizeof(buf));
                PyErr_CheckSignals();
            }

            if (cqe->user_data == WAKEUP_FD_TAG) {
                uint64_t val;
                read(self->wakeup_fd, &val, sizeof(val));

                struct io_uring_sqe *sqe = io_uring_get_sqe(self->ring);
                if (sqe) {
                    io_uring_prep_read(sqe, self->wakeup_fd, &self->wakeup_buf, sizeof(uint64_t), 0);
                    io_uring_sqe_set_data64(sqe, WAKEUP_FD_TAG);
                }
                io_uring_cqe_seen(self->ring, cqe);
                continue;
            }

            switch (slot->opcode) {
            case IORING_OP_READ:
                if (slot->buffer_payload->linear->buffer) {
                    result = PyBytes_FromStringAndSize(slot->buffer_payload->linear->buffer, cqe->res);
                }
                if (slot->buffer_payload->mode == FIXED) {
                    release_buffer_idx(slot->buffer_payload->idx_registry, slot->buffer_payload->buf_idx);
                }
                free_buffer_payload(slot->buffer_payload, false);
                slot->buffer_payload = NULL;
                break;
            case IORING_OP_READV:
                vec = slot->buffer_payload->vector;
                remaining = cqe->res;

                result = PyBytes_FromStringAndSize(NULL, remaining);
                if (result) {
                    char *dst = PyBytes_AS_STRING(result);
                    for (uint32_t i = 0; i < vec->nr_vecs && remaining > 0; i++) {
                        size_t chunk = vec->iovecs[i].iov_len;
                        if ((Py_ssize_t)chunk > remaining) {
                            chunk = (size_t)remaining;
                        }
                        memcpy(dst, vec->iovecs[i].iov_base, chunk);
                        dst += chunk;
                        remaining -= (Py_ssize_t)chunk;
                    }
                }
                free_buffer_payload(slot->buffer_payload, false);
                slot->buffer_payload = NULL;
                break;
            case IORING_OP_WRITE:
                result = PyLong_FromLong(cqe->res);

                if (slot->buffer_payload->mode == FIXED) {
                    release_buffer_idx(slot->buffer_payload->idx_registry, slot->buffer_payload->buf_idx);
                }
                free_buffer_payload(slot->buffer_payload, true);
                slot->buffer_payload = NULL;
                break;
            case IORING_OP_WRITEV:
                result = PyLong_FromLong(cqe->res);
                if (slot->buffer_payload) {
                    free_buffer_payload(slot->buffer_payload, false);
                    slot->buffer_payload = NULL;
                }
                break;
            case IORING_OP_OPENAT2:
                if (slot->file) {
                    PuringFile *file = (PuringFile *)slot->file;
                    file->fd = cqe->res;
                    result = (PyObject *)slot->file;
                }
                break;
            case IORING_OP_SOCKET:
                if (slot->socket) {
                    PuringSocket *sock = (PuringSocket *)slot->socket;
                    sock->sock_fd = cqe->res;
                    SOCKET_STATES state = NEW;
                    sock->state = state;
                    result = (PyObject *)slot->socket;
                }
                break;
            case IORING_OP_BIND:
                if (slot->socket) {
                    SOCKET_STATES state = BOUND;
                    slot->socket->state = state;
                    result = PyLong_FromLong(cqe->res);
                }
                break;
            case IORING_OP_CONNECT:
                if (slot->socket) {
                    SOCKET_STATES state = CONNECTED;
                    slot->socket->state = state;
                    result = PyLong_FromLong(cqe->res);
                }
                break;
            case IORING_OP_LISTEN:
                if (slot->socket) {
                    SOCKET_STATES state = LISTENING;
                    slot->socket->state = state;
                    result = PyLong_FromLong(cqe->res);
                }
                break;
            case IORING_OP_ACCEPT:
                if (slot->socket) {
                    struct sockaddr_storage *peer_addr = (struct sockaddr_storage *)slot->addr;

                    PuringSocket *conn = PyObject_GC_New(PuringSocket, &PuringSocketType);
                    if (!conn) {
                        PyErr_SetString(PyExc_RuntimeError, "Can't create socket");
                        PyErr_Print();
                        free(peer_addr);
                        io_uring_cqe_seen(self->ring, cqe);
                        continue;
                    }
                    conn->sock_fd = cqe->res;
                    conn->closed = false;
                    conn->loop = slot->socket->loop;
                    Py_INCREF(conn->loop);
                    conn->domain = slot->socket->domain;
                    conn->state = ACCEPTING;

                    conn->addr = (struct sockaddr *)peer_addr;

                    slot->buffer_payload = NULL;

                    PyObject_GC_Track(conn);
                    result = (PyObject *)conn;
                }
                break;
            case IORING_OP_RECV:
                if (slot->socket) {
                    if (cqe->res == 0) {
                        result = PyBytes_FromStringAndSize(NULL, 0);
                    } else if (cqe->res > 0) {
                        void *buffer = slot->buffer_payload->linear->buffer;
                        result = PyBytes_FromStringAndSize((char *)buffer, cqe->res);
                    }
                    if (slot->buffer_payload->mode == FIXED) {
                        release_buffer_idx(slot->buffer_payload->idx_registry, slot->buffer_payload->buf_idx);
                    }
                    free_buffer_payload(slot->buffer_payload, false);
                    slot->buffer_payload = NULL;
                }
                break;
            case IORING_OP_RECVMSG:
                if (slot->buffer_payload->mode == FIXED) {
                    release_buffer_idx(slot->buffer_payload->idx_registry, slot->buffer_payload->buf_idx);
                }

                if (slot->stream_strategy == MULTISHOT) {
                    unsigned bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
                    void *buf = (char *)slot->buffer_payload->linear->buffer +
                                (bid * slot->buffer_payload->linear->len);
                    RecvMsgMultishotResult out = puring_recvmsg_validate_multishot(
                        buf, cqe->res, slot->msghdr, cqe->res
                    );
                    if (out.is_null == true) {
                        PyErr_SetString(PyExc_RuntimeError, "Operation result is not valid.");
                        PyErr_Print();
                    }
                    if (is_puring_recvmsg_multishot_resubmit_required(cqe)) {
                        TimeoutParams timeout_params = {0};
                        puring_recvmsg_multishot(
                            slot->socket->loop->ring,
                            index,
                            slot->socket->sock_fd,
                            slot->buffer_payload->bgid,
                            0,
                            slot->msghdr,
                            timeout_params
                        );
                    }
                    result = PyBytes_FromStringAndSize(out.payload, (Py_ssize_t)out.payload_len);
                    break;
                }

                if (slot->buffer_payload->payload_type == PAYLOAD_LINEAR) {
                    result = PyBytes_FromStringAndSize((char *)slot->buffer_payload->linear->buffer, cqe->res);
                    if (slot->addr) {
                        free(slot->addr);
                        slot->addr = NULL;
                    }
                } else {
                    vec = slot->buffer_payload->vector;
                    remaining = cqe->res;
                    result = PyBytes_FromStringAndSize(NULL, remaining);
                    if (result) {
                        char *dst = PyBytes_AS_STRING(result);
                        for (uint32_t i = 0; i < vec->nr_vecs && remaining > 0; i++) {
                            size_t chunk = vec->iovecs[i].iov_len;
                            if ((Py_ssize_t)chunk > remaining) {
                                chunk = (size_t)remaining;
                            }
                            memcpy(dst, vec->iovecs[i].iov_base, chunk);
                            dst += chunk;
                            remaining -= (Py_ssize_t)chunk;
                        }
                    }
                }

                free_buffer_payload(slot->buffer_payload, false);
                slot->buffer_payload = NULL;
                break;
            case IORING_OP_SEND:
                if (slot->buffer_payload) {
                    if (slot->buffer_payload->mode == FIXED) {
                        release_buffer_idx(slot->buffer_payload->idx_registry, slot->buffer_payload->buf_idx);
                    }
                    free_buffer_payload(slot->buffer_payload, false);
                    slot->buffer_payload = NULL;
                }
                result = PyLong_FromLong(cqe->res);
                break;
            case IORING_OP_SENDMSG:
                if (slot->buffer_payload) {
                    if (slot->buffer_payload->mode == FIXED) {
                        release_buffer_idx(slot->buffer_payload->idx_registry, slot->buffer_payload->buf_idx);
                    }
                    free_buffer_payload(slot->buffer_payload, false);
                    slot->buffer_payload = NULL;
                }
                result = PyLong_FromLong(cqe->res);
                break;
            case IORING_OP_CLOSE:
                if (slot->socket) {
                    SOCKET_STATES state = CLOSED;
                    slot->socket->state = state;
                    slot->socket->closed = true;
                    result = PyLong_FromLong(cqe->res);
                } else if (slot->file) {
                    slot->file->closed = true;
                    result = PyLong_FromLong(cqe->res);
                }
                break;
            case IORING_OP_TIMEOUT:
                result = Py_None;
                Py_INCREF(result);
                break;
            default:
                result = PyLong_FromLong(cqe->res);
            }
        }

        if (result) {
            PyObject *res_call = PyObject_CallMethod(slot->future, "set_result", "O", result);
            Py_XDECREF(res_call);
            Py_DECREF(result);
        }

        registry_remove(self->registry, index);
        io_uring_cqe_seen(self->ring, cqe);
    }
}
