#pragma once

#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
    #define SVAROG_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define SVAROG_PLATFORM_LINUX
#elif defined(__APPLE__) && defined(__MACH__)
    #define SVAROG_PLATFORM_MACOS
#elif defined(__FreeBSD__)
    #define SVAROG_PLATFORM_FREEBSD
    #define SVAROG_PLATFORM_BSD
#elif defined(__OpenBSD__)
    #define SVAROG_PLATFORM_OPENBSD
    #define SVAROG_PLATFORM_BSD
#elif defined(__NetBSD__)
    #define SVAROG_PLATFORM_NETBSD
    #define SVAROG_PLATFORM_BSD
#elif defined(__DragonFly__)
    #define SVAROG_PLATFORM_DRAGONFLY
    #define SVAROG_PLATFORM_BSD
#elif defined(BSD)
    #define SVAROG_PLATFORM_BSD
#endif

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif

namespace svarog::io::detail {
enum class platform : std::uint8_t {
    bsd,
    linux_os,
    macos,
    windows,
    unknown
};

constexpr platform current_platform() noexcept {
#if defined(SVAROG_PLATFORM_WINDOWS)
    return platform::windows;
#elif defined(SVAROG_PLATFORM_LINUX)
    return platform::linux_os;
#elif defined(__APPLE__) && defined(__MACH__)
    return platform::macos;
#elif defined(SVAROG_PLATFORM_BSD)
    return platform::bsd;
#else
    return platform::unknown;
#endif
}

constexpr bool is_bsd() noexcept {
    return current_platform() == platform::bsd;
}
constexpr bool is_linux() noexcept {
    return current_platform() == platform::linux_os;
}
constexpr bool is_macos() noexcept {
    return current_platform() == platform::macos;
}
constexpr bool is_windows() noexcept {
    return current_platform() == platform::windows;
}

enum class reactor_backend : std::uint8_t {
    epoll,
    kqueue,
    iocp,
    select_poll
};

constexpr reactor_backend default_rector() noexcept {
    constexpr auto platform = current_platform();

    switch (platform) {
    case platform::linux_os:
        return reactor_backend::epoll;
        break;
    case platform::macos:
    case platform::bsd:
        return reactor_backend::kqueue;
        break;
    case platform::windows:
        return reactor_backend::iocp;
        break;
    default:
        return reactor_backend::select_poll;
        break;
    }
}
}  // namespace svarog::io::detail
