#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "svarog/execution/strand.hpp"
#include "svarog/execution/thread_pool.hpp"
#include "svarog/io/io_context.hpp"

using namespace svarog::io;
using namespace svarog::execution;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Strand Serialization Example ===\n\n";

    // Create io_context with 4 worker threads
    thread_pool pool(4);

    // Create a strand to serialize access to shared state
    strand<io_context::executor_type> s1(pool.get_executor());

    // Shared counter (no mutex needed - strand provides serialization!)
    std::atomic<int> counter{0};
    std::atomic<int> max_concurrent{0};
    std::atomic<int> current_concurrent{0};

    std::cout << "Posting 100 tasks to strand...\n";

    // Post 100 tasks that would race without serialization
    for (int i = 0; i < 100; ++i) {
        s1.post([&, task_num = i] {
            // Track concurrency (should always be 1 for same strand)
            int current = ++current_concurrent;
            int expected_max = max_concurrent.load();
            while (current > expected_max && !max_concurrent.compare_exchange_weak(expected_max, current)) {
            }

            // Simulate work
            int old = counter.load();
            std::this_thread::sleep_for(10us);  // Force contention
            counter.store(old + 1);

            --current_concurrent;
        });
    }

    // Wait for all work to complete
    std::this_thread::sleep_for(200ms);
    pool.stop();

    std::cout << "\nResults:\n";
    std::cout << "  Counter value: " << counter.load() << " (expected: 100)\n";
    std::cout << "  Max concurrent executions: " << max_concurrent.load() << " (expected: 1 for serialization)\n";

    if (counter.load() == 100 && max_concurrent.load() == 1) {
        std::cout << "\n✅ SUCCESS: Strand correctly serialized execution!\n";
        return 0;
    } else {
        std::cout << "\n❌ FAILURE: Serialization broken!\n";
        return 1;
    }
}
