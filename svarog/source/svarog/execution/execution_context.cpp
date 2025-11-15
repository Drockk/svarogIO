#include "svarog/execution/execution_context.hpp"

#include <ranges>

namespace svarog::execution {

execution_context::~execution_context() {
    // Execute cleanup callbacks in reverse order using ranges::reverse_view
    // This ensures proper shutdown hook calls and destruction order
    for (auto& cleanup : std::ranges::reverse_view(m_cleanup_callbacks)) {
        cleanup();
    }
}

}  // namespace svarog::execution
