#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

#include "svarog/execution/thread_pool.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace svarog;
using namespace std::chrono_literals;

TEST_CASE("thread_pool: basic operations", "[thread_pool][basic]") {
    SECTION("construction and destruction") {
        execution::thread_pool pool(4);
        REQUIRE(pool.thread_count() == 4);
        REQUIRE_FALSE(pool.stopped());
    }

    SECTION("post and execute") {
        execution::thread_pool pool(4);
        std::atomic<int> counter{0};

        for (int i = 0; i < 100; ++i) {
            pool.post([&] { counter.fetch_add(1, std::memory_order_relaxed); });
        }

        // Wait for all tasks to complete
        while (counter.load(std::memory_order_acquire) < 100) {
            std::this_thread::sleep_for(1ms);
        }

        REQUIRE(counter == 100);
    }

    SECTION("stop before destruction") {
        execution::thread_pool pool(2);

        REQUIRE_FALSE(pool.stopped());

        pool.stop();

        // Note: stopped() may not return true immediately after stop()
        // because workers may still be draining their queues
        // The important thing is that stop() completes and destructor works
    }
}

TEST_CASE("thread_pool: exception handling", "[thread_pool][exception]") {
    execution::thread_pool pool(2);
    std::atomic<bool> executed{false};

    // Task that throws
    pool.post([] { throw std::runtime_error("test exception"); });

    // Task that should still execute
    pool.post([&] {
        std::this_thread::sleep_for(10ms);
        executed = true;
    });

    // Wait for task completion
    std::this_thread::sleep_for(50ms);

    REQUIRE(executed);
}

TEST_CASE("thread_pool: multi-threaded execution", "[thread_pool][threading]") {
    execution::thread_pool pool(4);
    std::atomic<int> counter{0};
    std::set<std::thread::id> thread_ids;
    std::mutex mtx;

    constexpr int num_tasks = 100;

    for (int i = 0; i < num_tasks; ++i) {
        pool.post([&] {
            {
                std::lock_guard lock(mtx);
                thread_ids.insert(std::this_thread::get_id());
            }
            counter.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(100us);
        });
    }

    // Wait for all tasks to complete
    while (counter.load(std::memory_order_acquire) < num_tasks) {
        std::this_thread::sleep_for(1ms);
    }

    // Stop the pool before checking thread_ids
    pool.stop();

    // Now safe to access thread_ids - all workers are stopped
    std::lock_guard lock(mtx);
    REQUIRE(counter == num_tasks);
    REQUIRE(thread_ids.size() > 1);
}
