#include <atomic>
#include <chrono>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "svarog/execution/strand.hpp"
#include "svarog/execution/thread_pool.hpp"
#include "svarog/execution/work_queue.hpp"
#include "svarog/io/io_context.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace svarog::execution;
using namespace svarog::io;
using namespace std::chrono_literals;

// ============================================================================
// io_context + work_queue Integration Tests
// ============================================================================

TEST_CASE("integration: io_context + work_queue", "[integration][io_context][work_queue]") {
    SECTION("io_context uses work_queue internally") {
        io_context ctx;
        std::atomic<int> counter{0};

        // Post work to io_context (internally uses work_queue)
        for (int i = 0; i < 100; ++i) {
            ctx.post([&] { counter++; });
        }

        // Run event loop
        std::thread worker([&] { ctx.run(); });

        // Wait for work to complete
        std::this_thread::sleep_for(50ms);
        ctx.stop();
        worker.join();

        REQUIRE(counter == 100);
    }

    SECTION("multiple io_contexts with separate work_queues") {
        io_context ctx1;
        io_context ctx2;

        std::atomic<int> counter1{0};
        std::atomic<int> counter2{0};

        // Post to different contexts
        for (int i = 0; i < 50; ++i) {
            ctx1.post([&] { counter1++; });
            ctx2.post([&] { counter2++; });
        }

        // Run both contexts
        std::thread worker1([&] { ctx1.run(); });
        std::thread worker2([&] { ctx2.run(); });

        std::this_thread::sleep_for(50ms);
        ctx1.stop();
        ctx2.stop();
        worker1.join();
        worker2.join();

        REQUIRE(counter1 == 50);
        REQUIRE(counter2 == 50);
    }

    SECTION("work_queue is standalone (no dependency on execution_context)") {
        work_queue queue;
        std::atomic<int> counter{0};

        // Use work_queue independently
        for (int i = 0; i < 100; ++i) {
            [[maybe_unused]] bool pushed = queue.push([&counter] { counter++; });
        }

        // Process items manually (no io_context needed)
        std::thread worker([&] {
            while (auto result = queue.try_pop()) {
                if (result.has_value()) {
                    (*result)();
                } else {
                    break;
                }
            }
        });

        std::this_thread::sleep_for(50ms);
        queue.stop();
        worker.join();

        REQUIRE(counter == 100);
    }
}

// ============================================================================
// io_context + strand Integration Tests
// ============================================================================

TEST_CASE("integration: io_context + strand", "[integration][io_context][strand]") {
    SECTION("strands wrapping io_context executors") {
        thread_pool pool(4);
        strand<io_context::executor_type> s(pool.get_executor());

        std::atomic<int> counter{0};
        std::atomic<int> max_concurrent{0};
        std::atomic<int> current_concurrent{0};

        for (int i = 0; i < 500; ++i) {
            s.post([&] {
                int current = ++current_concurrent;
                int expected = max_concurrent.load();
                while (current > expected && !max_concurrent.compare_exchange_weak(expected, current)) {
                }

                counter++;
                std::this_thread::sleep_for(10us);

                --current_concurrent;
            });
        }

        std::this_thread::sleep_for(200ms);
        pool.stop();

        REQUIRE(counter == 500);
        REQUIRE(max_concurrent == 1);  // Strand serialization verified
    }

    SECTION("multiple strands in same io_context") {
        thread_pool pool(4);
        strand<io_context::executor_type> s1(pool.get_executor());
        strand<io_context::executor_type> s2(pool.get_executor());
        strand<io_context::executor_type> s3(pool.get_executor());

        std::atomic<int> counter1{0};
        std::atomic<int> counter2{0};
        std::atomic<int> counter3{0};

        // Post to all strands
        for (int i = 0; i < 100; ++i) {
            s1.post([&] { counter1++; });
            s2.post([&] { counter2++; });
            s3.post([&] { counter3++; });
        }

        std::this_thread::sleep_for(100ms);
        pool.stop();

        REQUIRE(counter1 == 100);
        REQUIRE(counter2 == 100);
        REQUIRE(counter3 == 100);
    }

    SECTION("concurrent run() with strands") {
        thread_pool pool(4);  // 4 threads calling run()
        strand<io_context::executor_type> s1(pool.get_executor());
        strand<io_context::executor_type> s2(pool.get_executor());

        std::atomic<int> total_executed{0};

        // Post lots of work
        for (int i = 0; i < 1000; ++i) {
            s1.post([&] { total_executed++; });
            s2.post([&] { total_executed++; });
        }

        std::this_thread::sleep_for(200ms);
        pool.stop();

        REQUIRE(total_executed == 2000);
    }
}

// ============================================================================
// All Components Together - Full Integration
// ============================================================================

TEST_CASE("integration: Phase 1 full integration", "[integration][full]") {
    thread_pool pool(4);

    strand<io_context::executor_type> s1(pool.get_executor());
    strand<io_context::executor_type> s2(pool.get_executor());

    std::atomic<int> s1_counter{0};
    std::atomic<int> s2_counter{0};
    std::atomic<int> s1_max_concurrent{0};
    std::atomic<int> s2_max_concurrent{0};
    std::atomic<int> s1_current{0};
    std::atomic<int> s2_current{0};

    // Post work to both strands
    for (int i = 0; i < 500; ++i) {
        s1.post([&] {
            int current = ++s1_current;
            int expected = s1_max_concurrent.load();
            while (current > expected && !s1_max_concurrent.compare_exchange_weak(expected, current)) {
            }

            s1_counter++;
            std::this_thread::sleep_for(5us);

            --s1_current;
        });

        s2.post([&] {
            int current = ++s2_current;
            int expected = s2_max_concurrent.load();
            while (current > expected && !s2_max_concurrent.compare_exchange_weak(expected, current)) {
            }

            s2_counter++;
            std::this_thread::sleep_for(5us);

            --s2_current;
        });
    }

    std::this_thread::sleep_for(200ms);
    pool.stop();

    // Verify work completed
    REQUIRE(s1_counter == 500);
    REQUIRE(s2_counter == 500);

    // Verify serialization within each strand
    REQUIRE(s1_max_concurrent == 1);
    REQUIRE(s2_max_concurrent == 1);
}

// ============================================================================
// Real-World Scenarios
// ============================================================================

TEST_CASE("integration: producer-consumer pattern", "[integration][patterns]") {
    thread_pool pool(4);

    strand<io_context::executor_type> producer_strand(pool.get_executor());
    strand<io_context::executor_type> consumer_strand(pool.get_executor());

    std::queue<int> shared_queue;
    std::mutex queue_mutex;
    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};

    constexpr int num_items = 1000;

    // Producer: generates items
    for (int i = 0; i < num_items; ++i) {
        producer_strand.post([&, item = i] {
            {
                std::lock_guard lock(queue_mutex);
                shared_queue.push(item);
            }
            items_produced++;

            // Notify consumer
            consumer_strand.post([&] {
                int item_to_consume = -1;
                {
                    std::lock_guard lock(queue_mutex);
                    if (!shared_queue.empty()) {
                        item_to_consume = shared_queue.front();
                        shared_queue.pop();
                    }
                }
                if (item_to_consume != -1) {
                    items_consumed++;
                }
            });
        });
    }

    std::this_thread::sleep_for(300ms);
    pool.stop();

    REQUIRE(items_produced == num_items);
    REQUIRE(items_consumed == num_items);
}

TEST_CASE("integration: pipeline pattern", "[integration][patterns]") {
    thread_pool pool(4);

    strand<io_context::executor_type> stage1(pool.get_executor());
    strand<io_context::executor_type> stage2(pool.get_executor());
    strand<io_context::executor_type> stage3(pool.get_executor());

    std::atomic<int> stage1_count{0};
    std::atomic<int> stage2_count{0};
    std::atomic<int> stage3_count{0};

    constexpr int num_items = 100;

    // Stage 1 → Stage 2 → Stage 3 pipeline
    for (int i = 0; i < num_items; ++i) {
        stage1.post([&, item = i] {
            // Process in stage 1
            int processed = item * 2;
            stage1_count++;

            // Pass to stage 2
            stage2.post([&, processed] {
                // Process in stage 2
                int further_processed = processed + 10;
                stage2_count++;

                // Pass to stage 3
                stage3.post([&, further_processed] {
                    // Final processing in stage 3
                    [[maybe_unused]] int final = further_processed * 3;
                    stage3_count++;
                });
            });
        });
    }

    std::this_thread::sleep_for(200ms);
    pool.stop();

    REQUIRE(stage1_count == num_items);
    REQUIRE(stage2_count == num_items);
    REQUIRE(stage3_count == num_items);
}

TEST_CASE("integration: task scheduler simulation", "[integration][patterns]") {
    thread_pool pool(2);
    strand<io_context::executor_type> scheduler(pool.get_executor());

    std::atomic<int> immediate_tasks{0};
    std::atomic<int> delayed_tasks{0};

    // Simulate immediate tasks
    for (int i = 0; i < 50; ++i) {
        scheduler.post([&] { immediate_tasks++; });
    }

    // Simulate delayed tasks (posted after a delay)
    std::thread delayer([&] {
        std::this_thread::sleep_for(50ms);
        for (int i = 0; i < 50; ++i) {
            scheduler.post([&] { delayed_tasks++; });
        }
    });

    delayer.join();
    std::this_thread::sleep_for(100ms);
    pool.stop();

    REQUIRE(immediate_tasks == 50);
    REQUIRE(delayed_tasks == 50);
}

TEST_CASE("integration: exception propagation across components", "[integration][exceptions]") {
    thread_pool pool(2);
    strand<io_context::executor_type> s(pool.get_executor());

    std::atomic<int> before_exception{0};
    std::atomic<int> after_exception{0};

    s.post([&] { before_exception++; });

    s.post([&] {
        before_exception++;
        throw std::runtime_error("test exception");
    });

    s.post([&] { after_exception++; });
    s.post([&] { after_exception++; });

    std::this_thread::sleep_for(50ms);
    pool.stop();

    // Strand should continue after exception
    REQUIRE(before_exception == 2);
    REQUIRE(after_exception == 2);
}
