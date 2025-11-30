
#include "svarog/io/detail/iocp_reactor.hpp"

namespace svarog::io::detail {

iocp_reactor::iocp_reactor() {
    winsock_initializer::ensure_initialized();

    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (m_iocp == nullptr) {
        throw std::system_error(GetLastError(), std::system_category(), "CreateIoCompletionPort failed");
    }
}

void iocp_reactor::associate_socket(native_handle_type socket) {
    HANDLE result =
        CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), m_iocp, iocp_op_codes::completion_key_io, 0);
    if (result == nullptr) {
        throw std::system_error(GetLastError(), std::system_category(), "Failed to associate socket with IOCP");
    }
}

void iocp_reactor::start_read(native_handle_type socket, void* buffer, std::size_t size, completion_handler handler) {
    auto* op = acquire_operation();
    op->handler = std::move(handler);
    op->type = io_operation::read;
    op->socket = socket;

    WSABUF wsabuf;
    wsabuf.buf = static_cast<char*>(buffer);
    wsabuf.len = static_cast<ULONG>(size);

    DWORD flags = 0;
    int result = WSARecv(socket, &wsabuf, 1, nullptr, &flags, op, nullptr);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            release_operation(op);
            throw std::system_error(error, std::system_category(), "WSARecv failed");
        }
    }
    // Operation pending - completion will be posted to IOCP
}

std::size_t iocp_reactor::do_run_one(std::chrono::milliseconds timeout) {
    DWORD bytes_transferred = 0;
    ULONG_PTR completion_key = 0;
    OVERLAPPED* overlapped = nullptr;

    BOOL result = GetQueuedCompletionStatus(m_iocp, &bytes_transferred, &completion_key, &overlapped,
                                            static_cast<DWORD>(timeout.count()));

    if (!result && overlapped == nullptr) {
        DWORD error = GetLastError();
        if (error == WAIT_TIMEOUT)
            return 0;
        throw std::system_error(error, std::system_category(), "GetQueuedCompletionStatus failed");
    }

    if (completion_key == iocp_op_codes::completion_key_stop) {
        return 0;  // Stop signal
    }

    auto* op = static_cast<iocp_operation*>(overlapped);
    std::error_code ec;

    if (!result) {
        ec = std::error_code(GetLastError(), std::system_category());
    }

    // Move handler before releasing operation
    auto handler = std::move(op->handler);
    release_operation(op);

    if (handler) {
        handler(ec, bytes_transferred);
        return 1;
    }
    return 0;
}

void iocp_reactor::do_stop() {
    m_stopped.store(true, std::memory_order_release);
    // Post stop notification
    PostQueuedCompletionStatus(m_iocp, 0, iocp_op_codes::completion_key_stop, nullptr);
}
}  // namespace svarog::io::detail
