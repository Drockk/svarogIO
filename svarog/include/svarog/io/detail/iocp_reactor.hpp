#ifdef SVAROG_PLATFORM_WINDOWS

namespace svarog::io::detail {

struct iocp_operation : OVERLAPPED {
    completion_handler handler;
    io_operation type;
    native_handle_type socket;

    iocp_operation() {
        std::memset(static_cast<OVERLAPPED*>(this), 0, sizeof(OVERLAPPED));
    }
};

struct iocp_op_codes {
    static constexpr ULONG_PTR completion_key_io = 1;
    static constexpr ULONG_PTR completion_key_stop = 2;
    static constexpr ULONG_PTR completion_key_timer = 3;
};

class iocp_reactor : public reactor_base<iocp_reactor> {
public:
    iocp_reactor();
    ~iocp_reactor();

    iocp_reactor(const iocp_reactor&) = delete;
    iocp_reactor& operator=(const iocp_reactor&) = delete;

    void associate_socket(native_handle_type socket);

    void start_read(native_handle_type socket, void* buffer, std::size_t size, completion_handler handler);
    void start_write(native_handle_type socket, const void* buffer, std::size_t size, completion_handler handler);
    void start_accept(native_handle_type listen_socket, native_handle_type accept_socket, completion_handler handler);
    void start_connect(native_handle_type socket, const sockaddr* addr, int addrlen, completion_handler handler);

private:
    friend class reactor_base<iocp_reactor>;

    void do_register(native_handle_type fd, io_operation ops, completion_handler handler);
    void do_unregister(native_handle_type fd);
    void do_modify(native_handle_type fd, io_operation ops);
    std::size_t do_run_one(std::chrono::milliseconds timeout);
    std::size_t do_poll_one();
    void do_stop();
    [[nodiscard]] bool is_stopped() const noexcept;

    HANDLE m_iocp{INVALID_HANDLE_VALUE};
    std::atomic<bool> m_stopped{false};

    std::vector<std::unique_ptr<iocp_operation>> m_operation_pool;
    std::mutex m_pool_mutex;

    iocp_operation* acquire_operation();
    void release_operation(iocp_operation* op);
};

class winsock_initializer {
public:
    winsock_initializer() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            throw std::system_error(result, std::system_category(), "WSAStartup failed");
        }
    }

    ~winsock_initializer() {
        WSACleanup();
    }

    static void ensure_initialized() {
        static winsock_initializer instance;
    }
};

}  // namespace svarog::io::detail

#endif  // SVAROG_PLATFORM_WINDOWS
