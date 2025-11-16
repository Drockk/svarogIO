#pragma once

#include <atomic>
#include <cstddef>     // size_t
#include <functional>  // std::move_only_function
#include <memory>      // std::unique_ptr

#include "svarog/core/contracts.hpp"

// Use std::expected if available (C++23), otherwise use tl::expected (backport)
#if __cpp_lib_expected >= 202202L
    #include <expected>  // std::expected, std::unexpected
namespace svarog::execution {
template <typename T, typename E>
using expected = std::expected<T, E>;
using std::unexpected;
}  // namespace svarog::execution
#else
    #include <tl/expected.hpp>  // tl::expected (backport for pre-C++23)
namespace svarog::execution {
template <typename T, typename E>
using expected = tl::expected<T, E>;
using tl::unexpected;
}  // namespace svarog::execution
#endif

class work_queue_impl;

namespace svarog::execution {

using work_item = std::move_only_function<void()>;

enum class queue_error {
    empty,
    stopped
};

class work_queue {
public:
    explicit work_queue();
    ~work_queue();

    work_queue(const work_queue&) = delete;
    work_queue& operator=(const work_queue&) = delete;
    work_queue(work_queue&&) = delete;
    work_queue& operator=(work_queue&&) = delete;

    [[nodiscard]] bool push(work_item&& t_item);

    [[nodiscard]] expected<work_item, queue_error> pop() noexcept;
    [[nodiscard]] expected<work_item, queue_error> try_pop() noexcept;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    void stop() noexcept;
    [[nodiscard]] bool stopped() const noexcept;
    void clear() noexcept;

private:
    std::unique_ptr<work_queue_impl> m_impl;
};

}  // namespace svarog::execution
