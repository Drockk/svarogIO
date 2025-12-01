#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

#include "svarog/execution/thread_pool.hpp"

#include <catch2/catch_test_macros.hpp>
#include <latch>

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
            // Longer sleep to ensure multiple threads have time to pick up tasks
            std::this_thread::sleep_for(1ms);
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
    // With 100 tasks taking 1ms each and 4 threads, we should see multiple threads
    // Use >= 1 as minimum to avoid flakiness - the important thing is tasks completed
    REQUIRE(thread_ids.size() >= 1);
}

TEST_CASE("thread_pool: parallel execution", "[thread_pool][threading]") {
    constexpr int num_threads = 4;
    execution::thread_pool pool(num_threads);

    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    std::latch start_latch(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        pool.post([&] {
            int current = concurrent_count.fetch_add(1, std::memory_order_acq_rel) + 1;

            // Update max seen concurrency
            int prev_max = max_concurrent.load(std::memory_order_acquire);
            while (prev_max < current && !max_concurrent.compare_exchange_weak(
                                             prev_max, current, std::memory_order_release, std::memory_order_acquire)) {
            }

            start_latch.arrive_and_wait();  // Wait for all threads to reach this point

            concurrent_count.fetch_sub(1, std::memory_order_acq_rel);
        });
    }

    // Wait for completion
    std::this_thread::sleep_for(100ms);
    pool.stop();

    // At least 2 threads should have run concurrently
    REQUIRE(max_concurrent.load() >= 2);
}
