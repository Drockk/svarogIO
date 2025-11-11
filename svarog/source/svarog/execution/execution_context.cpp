#include "svarog/execution/execution_context.hpp"

#include <ranges>

namespace svarog::execution {

execution_context::~execution_context() {
    // Execute cleanup callbacks in reverse order
    // This ensures proper shutdown hook calls and destruction order
    for (auto& cleanup : m_cleanup_callbacks | std::views::reverse) {
        cleanup();
    }
}

} // namespace svarog::execution
