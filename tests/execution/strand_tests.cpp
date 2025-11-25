#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "svarog/execution/strand.hpp"
#include "svarog/execution/thread_pool.hpp"
#include "svarog/io/io_context.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace svarog::execution;
using namespace svarog::io;
using namespace std::chrono_literals;

TEST_CASE("strand: serialization guarantee", "[strand][serialization]") {
    SECTION("handlers execute serially - no race conditions") {
        thread_pool pool(4);
        strand<io_context::executor_type> s(pool.get_executor());

        std::atomic<int> counter{0};
        std::atomic<int> max_concurrent{0};
        std::atomic<int> current_concurrent{0};
        std::atomic<int> completed{0};

        constexpr int num_tasks = 1000;

        // Post tasks that would race without serialization
        for (int i = 0; i < num_tasks; ++i) {
            s.post([&] {
                // Track concurrency
                int current = ++current_concurrent;
                int expected_max = max_concurrent.load();
                while (current > expected_max && !max_concurrent.compare_exchange_weak(expected_max, current)) {
                }

                // Simulate work that would race
                int old = counter.load();
                std::this_thread::sleep_for(1us);  // Force contention
                counter.store(old + 1);

                --current_concurrent;
                ++completed;
            });
        }

        // Wait for all tasks to complete
        while (completed.load() < num_tasks) {
            std::this_thread::sleep_for(1ms);
        }

        pool.stop();

        REQUIRE(counter == num_tasks);
        REQUIRE(max_concurrent == 1);  // Only 1 handler at a time
    }

    SECTION("multiple strands can run concurrently") {
        thread_pool pool(4);
        strand<io_context::executor_type> s1(pool.get_executor());
        strand<io_context::executor_type> s2(pool.get_executor());

        std::atomic<int> s1_max_concurrent{0};
        std::atomic<int> s1_current{0};
        std::atomic<int> s2_max_concurrent{0};
        std::atomic<int> s2_current{0};
        std::atomic<int> s1_completed{0};
        std::atomic<int> s2_completed{0};

        constexpr int num_tasks = 100;

        // Post to both strands
        for (int i = 0; i < num_tasks; ++i) {
            s1.post([&] {
                int current = ++s1_current;
                int expected = s1_max_concurrent.load();
                while (current > expected && !s1_max_concurrent.compare_exchange_weak(expected, current)) {
                }
                std::this_thread::sleep_for(10us);
                --s1_current;
                ++s1_completed;
            });

            s2.post([&] {
                int current = ++s2_current;
                int expected = s2_max_concurrent.load();
                while (current > expected && !s2_max_concurrent.compare_exchange_weak(expected, current)) {
                }
                std::this_thread::sleep_for(10us);
                --s2_current;
                ++s2_completed;
            });
        }

        // Wait for all tasks to complete
        while (s1_completed.load() < num_tasks || s2_completed.load() < num_tasks) {
            std::this_thread::sleep_for(1ms);
        }

        pool.stop();

        // Each strand individually serialized
        REQUIRE(s1_max_concurrent == 1);
        REQUIRE(s2_max_concurrent == 1);
    }
}

TEST_CASE("strand: FIFO ordering", "[strand][ordering]") {
    thread_pool pool(1);  // Single worker to ensure deterministic ordering
    strand<io_context::executor_type> s(pool.get_executor());

    constexpr int num_tasks = 100;
    std::vector<int> execution_order;
    std::mutex order_mutex;
    std::atomic<int> completed{0};

    // Post tasks in order
    for (int i = 0; i < num_tasks; ++i) {
        s.post([&, task_id = i] {
            std::lock_guard lock(order_mutex);
            execution_order.push_back(task_id);
            ++completed;
        });
    }

    // Wait for all tasks to complete
    while (completed.load() < num_tasks) {
        std::this_thread::sleep_for(1ms);
    }

    pool.stop();

    std::lock_guard lock(order_mutex);
    REQUIRE(execution_order.size() == num_tasks);

    // Verify FIFO order
    for (int i = 0; i < num_tasks; ++i) {
        REQUIRE(execution_order[static_cast<size_t>(i)] == i);
    }
}

TEST_CASE("strand: dispatch() immediate execution", "[strand][dispatch]") {
    thread_pool pool(2);
    strand<io_context::executor_type> s(pool.get_executor());

    std::atomic<bool> executed_immediately{false};
    std::atomic<bool> test_complete{false};

    SECTION("dispatch from strand thread executes immediately") {
        s.post([&] {
            // We're on strand thread now
            REQUIRE(s.running_in_this_thread());

            // dispatch() should execute immediately
            s.dispatch([&] {
                REQUIRE(s.running_in_this_thread());
                executed_immediately = true;
            });

            // Should have executed already
            REQUIRE(executed_immediately == true);
            test_complete = true;
        });

        // Wait for completion
        while (!test_complete.load()) {
            std::this_thread::sleep_for(1ms);
        }

        pool.stop();

        REQUIRE(test_complete == true);
    }

    SECTION("dispatch from other thread defers") {
        std::atomic<int> execution_count{0};

        // Post a long-running task to occupy the strand
        s.post([&] {
            std::this_thread::sleep_for(50ms);
            execution_count++;
        });

        // Give it time to start
        std::this_thread::sleep_for(10ms);

        // dispatch() from non-strand thread should defer (not execute immediately)
        REQUIRE_FALSE(s.running_in_this_thread());
        s.dispatch([&] { execution_count++; });

        // Should not have executed yet (strand is busy)
        std::this_thread::sleep_for(10ms);
        REQUIRE(execution_count <= 1);

        // Wait for both tasks to complete
        while (execution_count.load() < 2) {
            std::this_thread::sleep_for(1ms);
        }

        pool.stop();

        // Both should have executed eventually
        REQUIRE(execution_count == 2);
    }
}

TEST_CASE("strand: multi-threaded io_context", "[strand][multi-threading]") {
    thread_pool pool(4);
    strand<io_context::executor_type> s1(pool.get_executor());
    strand<io_context::executor_type> s2(pool.get_executor());

    std::atomic<int> s1_counter{0};
    std::atomic<int> s2_counter{0};

    constexpr int num_tasks = 500;

    // Post work to both strands from multiple threads
    std::vector<std::thread> posting_threads;
    posting_threads.reserve(4);
    for (int t = 0; t < 4; ++t) {
        posting_threads.emplace_back([&] {
            for (int i = 0; i < num_tasks / 4; ++i) {
                s1.post([&] { s1_counter++; });
                s2.post([&] { s2_counter++; });
            }
        });
    }

    for (auto& t : posting_threads) {
        t.join();
    }

    // Wait for all tasks to complete
    while (s1_counter.load() < num_tasks || s2_counter.load() < num_tasks) {
        std::this_thread::sleep_for(1ms);
    }

    pool.stop();

    REQUIRE(s1_counter == num_tasks);
    REQUIRE(s2_counter == num_tasks);
}

TEST_CASE("strand: exception handling", "[strand][exceptions]") {
    thread_pool pool(2);
    strand<io_context::executor_type> s(pool.get_executor());

    std::atomic<int> counter{0};

    // Post task that throws
    s.post([&] {
        counter++;
        throw std::runtime_error("test exception");
    });

    // Post more tasks - should still execute
    s.post([&] { counter++; });
    s.post([&] { counter++; });

    // Wait for all tasks to complete
    while (counter.load() < 3) {
        std::this_thread::sleep_for(1ms);
    }

    pool.stop();

    // All tasks should have executed (exceptions are caught)
    REQUIRE(counter == 3);
}

TEST_CASE("strand: recursion depth limit", "[strand][recursion]") {
    thread_pool pool(1);
    strand<io_context::executor_type> s(pool.get_executor());

    std::atomic<int> call_count{0};
    std::atomic<bool> exceeded_limit{false};

    // Test that deep recursion is handled gracefully
    // We'll post a task that dispatches itself recursively
    auto recursive_task = std::make_shared<std::function<void(int)>>();
    std::weak_ptr<std::function<void(int)>> weak_task = recursive_task;
    *recursive_task = [&, weak_task](int t_depth) {
        call_count++;

        if (t_depth < 150) {  // Try to recurse deeply
            if (auto task = weak_task.lock()) {
                s.dispatch([task, t_depth] { (*task)(t_depth + 1); });
            }
        } else {
            exceeded_limit = true;
        }
    };

    s.post([recursive_task] { (*recursive_task)(0); });

    // Wait for exceeded_limit to be set
    while (!exceeded_limit.load()) {
        std::this_thread::sleep_for(1ms);
    }

    pool.stop();

    // Should have completed all 150 calls
    // Some through dispatch (immediate), some through post (when limit hit)
    REQUIRE(call_count >= 150);
    REQUIRE(exceeded_limit == true);
}

TEST_CASE("strand: running_in_this_thread()", "[strand][thread-detection]") {
    thread_pool pool(2);
    strand<io_context::executor_type> s(pool.get_executor());

    std::atomic<bool> inside_check{false};
    std::atomic<bool> outside_check{false};
    std::atomic<bool> done{false};

    // From main thread - not on strand
    outside_check = s.running_in_this_thread();

    s.post([&] {
        // From strand thread - should return true
        inside_check = s.running_in_this_thread();
        done = true;
    });

    // Wait for task to complete
    while (!done.load()) {
        std::this_thread::sleep_for(1ms);
    }

    pool.stop();

    REQUIRE(outside_check == false);
    REQUIRE(inside_check == true);
}
