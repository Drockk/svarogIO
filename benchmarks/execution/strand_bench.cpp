#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "svarog/execution/strand.hpp"
#include "svarog/execution/thread_pool.hpp"
#include "svarog/io/io_context.hpp"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace svarog::execution;
using namespace svarog::io;
using namespace std::chrono_literals;

TEST_CASE("strand: serialization overhead", "[strand][bench][overhead]") {
    constexpr int num_tasks = 10000;

    SECTION("baseline: bare executor (no strand)") {
        thread_pool pool(4);
        std::atomic<int> completed{0};

        BENCHMARK("bare executor post") {
            completed = 0;
            for (int i = 0; i < num_tasks; ++i) {
                pool.get_executor().execute([&] { completed++; });
            }
            while (completed < num_tasks) {
                std::this_thread::yield();
            }
            return completed.load();
        };

        pool.stop();
    }

    SECTION("with strand (serialized)") {
        thread_pool pool(4);
        strand<io_context::executor_type> s(pool.get_executor());
        std::atomic<int> completed{0};

        BENCHMARK("strand post") {
            completed = 0;
            for (int i = 0; i < num_tasks; ++i) {
                s.post([&] { completed++; });
            }
            while (completed < num_tasks) {
                std::this_thread::yield();
            }
            return completed.load();
        };

        pool.stop();
    }

    // Expected: Strand overhead <50ns per task
    // With 10k tasks, total overhead should be <500µs
}

TEST_CASE("strand: dispatch vs post latency", "[strand][bench][latency]") {
    thread_pool pool(1);
    strand<io_context::executor_type> s(pool.get_executor());

    // Give thread pool time to start
    std::this_thread::sleep_for(10ms);

    SECTION("post latency (always defers)") {
        BENCHMARK("post latency") {
            std::promise<void> done;
            auto future = done.get_future();

            s.post([&] { done.set_value(); });

            future.wait();
        };
    }

    SECTION("dispatch latency (immediate when on strand)") {
        std::promise<void> ready_promise;
        std::promise<void> done_promise;
        auto ready_future = ready_promise.get_future();
        auto done_future = done_promise.get_future();

        s.post([&] {
            ready_promise.set_value();

            // Now we're on strand thread
            BENCHMARK("dispatch immediate") {
                int result = 0;
                s.dispatch([&] { result = 42; });
                return result;
            };

            done_promise.set_value();
        });

        // Wait for benchmark to start
        ready_future.wait();

        // Wait for benchmark to complete
        done_future.wait();

        // Give extra time for cleanup
        std::this_thread::sleep_for(100ms);
    }

    pool.stop();

    // Expected: dispatch immediate execution <10ns
}

TEST_CASE("strand: throughput with multiple strands", "[strand][bench][throughput]") {
    constexpr int num_strands = 4;
    constexpr int tasks_per_strand = 5000;

    thread_pool pool(4);
    std::vector<std::unique_ptr<strand<io_context::executor_type>>> strands;

    for (int i = 0; i < num_strands; ++i) {
        strands.push_back(std::make_unique<strand<io_context::executor_type>>(pool.get_executor()));
    }

    std::atomic<int> total_completed{0};

    BENCHMARK("4 strands parallel execution") {
        total_completed = 0;

        for (int s = 0; s < num_strands; ++s) {
            for (int i = 0; i < tasks_per_strand; ++i) {
                strands[static_cast<size_t>(s)]->post([&] { total_completed++; });
            }
        }

        while (total_completed < num_strands * tasks_per_strand) {
            std::this_thread::yield();
        }

        return total_completed.load();
    };

    pool.stop();

    // Expected: Throughput ≥80% of bare executor
    // With 4 strands on 4 workers, should achieve good parallelism
}

TEST_CASE("strand: contention handling", "[strand][bench][contention]") {
    constexpr int num_posting_threads = 8;
    constexpr int tasks_per_thread = 1000;

    thread_pool pool(2);
    strand<io_context::executor_type> s(pool.get_executor());

    std::atomic<int> completed{0};

    BENCHMARK("high contention posting") {
        completed = 0;
        std::vector<std::thread> threads;

        for (int t = 0; t < num_posting_threads; ++t) {
            threads.emplace_back([&] {
                for (int i = 0; i < tasks_per_thread; ++i) {
                    s.post([&] { completed++; });
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        while (completed < num_posting_threads * tasks_per_thread) {
            std::this_thread::yield();
        }

        return completed.load();
    };

    pool.stop();

    // Expected: Efficient handling of concurrent post() calls
    // Atomic CAS should scale well under contention
}

TEST_CASE("strand: serialization correctness under load", "[strand][bench][correctness]") {
    thread_pool pool(4);
    strand<io_context::executor_type> s(pool.get_executor());

    constexpr int num_tasks = 10000;
    std::atomic<int> counter{0};
    std::atomic<int> max_concurrent{0};
    std::atomic<int> current_concurrent{0};

    BENCHMARK("serialization under load") {
        counter = 0;
        max_concurrent = 0;
        current_concurrent = 0;

        for (int i = 0; i < num_tasks; ++i) {
            s.post([&] {
                int current = ++current_concurrent;
                int expected = max_concurrent.load();
                while (current > expected && !max_concurrent.compare_exchange_weak(expected, current)) {
                }

                // Simulate work
                int old = counter.load();
                counter.store(old + 1);

                --current_concurrent;
            });
        }

        while (counter < num_tasks) {
            std::this_thread::yield();
        }

        // Verify serialization held
        REQUIRE(max_concurrent == 1);
        return counter.load();
    };

    pool.stop();
}
