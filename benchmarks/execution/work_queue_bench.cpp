#include <iostream>
#include <thread>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <format>
#include <svarog/execution/work_queue.hpp>

TEST_CASE("work_queue: push throughput", "[work_queue][bench][throughput]") {
    svarog::execution::work_queue queue;
    std::atomic<int> counter{0};

    BENCHMARK("push single item") {
        std::ignore = queue.push([&counter] { counter++; });
    };

    BENCHMARK("push 10 items") {
        for (int i = 0; i < 10; ++i) {
            std::ignore = queue.push([&counter] { counter++; });
        }
    };

    BENCHMARK("push 100 items") {
        for (int i = 0; i < 100; ++i) {
            std::ignore = queue.push([&counter] { counter++; });
        }
    };

    BENCHMARK("push 1000 items") {
        for (int i = 0; i < 1000; ++i) {
            std::ignore = queue.push([&counter] { counter++; });
        }
    };
}

TEST_CASE("work_queue: pop throughput", "[work_queue][bench][throughput]") {
    svarog::execution::work_queue queue;
    std::atomic<int> counter{0};

    SECTION("try_pop performance") {
        // Pre-fill queue
        for (int i = 0; i < 10000; ++i) {
            std::ignore = queue.push([&counter] { counter++; });
        }

        BENCHMARK("try_pop single item") {
            auto result = queue.try_pop();
            if (result) {
                result.value()();  // Execute the lambda
            }
            return result.has_value();
        };
    }

    SECTION("blocking pop performance") {
        BENCHMARK("pop (blocking) single item") {
            // Pre-fill one item for each benchmark iteration
            std::ignore = queue.push([&counter] { counter++; });

            auto result = queue.pop();
            if (result) {
                result.value()();  // Execute the lambda
            }
            return result.has_value();
        };
    }
}

TEST_CASE("work_queue: producer-consumer latency", "[work_queue][bench][latency]") {
    using namespace std::chrono;

    svarog::execution::work_queue queue;
    std::vector<int64_t> latencies;
    latencies.reserve(10000);
    std::atomic<bool> consumer_running{true};

    // Consumer thread
    std::jthread consumer([&]() {
        while (consumer_running) {
            if (auto item = queue.try_pop()) {
                item.value()();  // Execute the lambda (which will record latency)
            } else {
                std::this_thread::yield();
            }
        }
    });

    BENCHMARK("end-to-end latency") {
        auto start = high_resolution_clock::now();
        std::ignore = queue.push([&latencies, start]() {
            auto end = high_resolution_clock::now();
            latencies.push_back(duration_cast<nanoseconds>(end - start).count());
        });
    };

    consumer_running = false;
    consumer.join();

    // Report statistics
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        auto p50 = latencies[latencies.size() / 2];
        auto p95 = latencies[latencies.size() * 95 / 100];
        auto p99 = latencies[latencies.size() * 99 / 100];

        std::cout << "Latency Statistics:\n";
        std::cout << std::format("  P50: {} ns\n", p50);
        std::cout << std::format("  P95: {} ns\n", p95);
        std::cout << std::format("  P99: {} ns\n", p99);
    }
}

TEST_CASE("work_queue: MPMC scalability", "[work_queue][bench][scalability]") {
    SECTION("1 producer, 1 consumer") {
        svarog::execution::work_queue queue;
        std::atomic<int> consumed{0};
        constexpr int items = 10000;

        BENCHMARK("1P/1C") {
            consumed = 0;

            std::jthread producer([&]() {
                for (int i = 0; i < items; ++i) {
                    std::ignore = queue.push([&consumed] { consumed++; });
                }
            });

            std::jthread consumer([&]() {
                while (consumed < items) {
                    if (auto item = queue.try_pop()) {
                        item.value()();  // Execute the lambda
                    }
                }
            });
        };
    }

    SECTION("4 producers, 4 consumers") {
        svarog::execution::work_queue queue;
        std::atomic<int> consumed{0};
        constexpr int items_per_producer = 2500;
        constexpr int total_items = items_per_producer * 4;

        BENCHMARK("4P/4C") {
            consumed = 0;

            std::vector<std::jthread> producers;
            for (int p = 0; p < 4; ++p) {
                producers.emplace_back([&]() {
                    for (int i = 0; i < items_per_producer; ++i) {
                        std::ignore = queue.push([&consumed] { consumed++; });
                    }
                });
            }

            std::vector<std::jthread> consumers;
            for (int c = 0; c < 4; ++c) {
                consumers.emplace_back([&]() {
                    while (consumed < total_items) {
                        if (auto item = queue.try_pop()) {
                            item.value()();  // Execute the lambda
                        }
                    }
                });
            }
        };
    }

    SECTION("8 producers, 8 consumers") {
        svarog::execution::work_queue queue;
        std::atomic<int> consumed{0};
        constexpr int items_per_producer = 1250;
        constexpr int total_items = items_per_producer * 8;

        BENCHMARK("8P/8C") {
            consumed = 0;

            std::vector<std::jthread> producers;
            for (int p = 0; p < 8; ++p) {
                producers.emplace_back([&]() {
                    for (int i = 0; i < items_per_producer; ++i) {
                        std::ignore = queue.push([&consumed] { consumed++; });
                    }
                });
            }

            std::vector<std::jthread> consumers;
            for (int c = 0; c < 8; ++c) {
                consumers.emplace_back([&]() {
                    while (consumed < total_items) {
                        if (auto item = queue.try_pop()) {
                            item.value()();  // Execute the lambda
                        }
                    }
                });
            }
        };
    }
}

TEST_CASE("work_queue: high contention", "[work_queue][bench][contention]") {
    const auto num_cores = std::thread::hardware_concurrency();

    SECTION("at hardware_concurrency") {
        svarog::execution::work_queue queue;
        std::atomic<int> total_ops{0};

        BENCHMARK("contention at HW threads") {
            total_ops = 0;

            std::vector<std::jthread> threads;
            for (uint32_t t = 0; t < num_cores; ++t) {
                threads.emplace_back([&]() {
                    for (int i = 0; i < 1000; ++i) {
                        std::ignore = queue.push([&total_ops] { total_ops++; });
                        total_ops++;
                        if (auto item = queue.try_pop()) {
                            item.value()();  // Execute the lambda
                            total_ops++;
                        }
                    }
                });
            }
        };
    }

    SECTION("2x hardware_concurrency (oversubscribed)") {
        svarog::execution::work_queue queue;
        std::atomic<int> total_ops{0};

        BENCHMARK("contention at 2x HW threads") {
            total_ops = 0;

            std::vector<std::jthread> threads;
            for (uint32_t t = 0; t < num_cores * 2; ++t) {
                threads.emplace_back([&]() {
                    for (int i = 0; i < 500; ++i) {
                        std::ignore = queue.push([&total_ops] { total_ops++; });
                        total_ops++;
                        if (auto item = queue.try_pop()) {
                            item.value()();  // Execute the lambda
                            total_ops++;
                        }
                    }
                });
            }
        };
    }
}
