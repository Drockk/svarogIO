#pragma once

#include "svarog/io/detail/platform_config.hpp"

#ifdef SVAROG_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    // POSIX (Linux, macOS, BSD)
    #include <arpa/inet.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

namespace svarog::network::detail {
#ifdef SVAROG_PLATFORM_WINDOWS
using native_socket_type = SOCKET;
inline constexpr native_socket_type invalid_socket = INVALID_SOCKET;
#else
using native_socket_type = int;
inline constexpr native_socket_type invalid_socket = -1;
#endif

inline int last_socket_error() noexcept {
#ifdef SVAROG_PLATFORM_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline int close_socket(native_socket_type t_socket) noexcept {
#ifdef SVAROG_PLATFORM_WINDOWS
    return ::closesocket(t_socket);
#else
    return ::close(t_socket);
#endif
}

inline int set_non_blocking(native_socket_type t_socket, bool t_non_blocking) noexcept {
#ifdef SVAROG_PLATFORM_WINDOWS
    u_long mode = t_non_blocking ? 1 : 0;
    return ::ioctlsocket(t_socket, FIONBIO, &mode);
#else
    auto flags = ::fcntl(t_socket, F_GETFL, 0);

    if (flags == -1) {
        return -1;
    }

    flags = t_non_blocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return ::fcntl(t_socket, F_SETFL, flags);
#endif
}

#if defined(SVAROG_PLATFORM_WINDOWS)
class wsa_initializer {
public:
    static wsa_initializer& instance() {
        static wsa_initializer inst;
        return inst;
    }

    bool initialized() const noexcept {
        return m_initialized;
    }

private:
    wsa_initializer() {
        WSADATA wsa_data;
        m_initialized = (WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0);
    }
    ~wsa_initializer() {
        if (m_initialized)
            WSACleanup();
    }

    wsa_initializer(const wsa_initializer&) = delete;
    wsa_initializer& operator=(const wsa_initializer&) = delete;

    bool m_initialized{false};
};

// Force initialization at startup
inline const bool wsa_init = wsa_initializer::instance().initialized();
#endif
}  // namespace svarog::network::detail
