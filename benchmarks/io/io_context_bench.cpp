#include <future>
#include <iostream>
#include <thread>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <format>
#include <latch>
#include <svarog/execution/work_guard.hpp>
#include <svarog/io/io_context.hpp>

TEST_CASE("io_context: post throughput - 1 worker", "[io_context][bench][throughput]") {
    svarog::io::io_context io_context;
    constexpr int64_t num_tasks = 10000;

    auto guard = svarog::execution::make_work_guard(io_context);
    std::jthread worker([&]() { io_context.run(); });

    BENCHMARK("post with 1 worker") {
        auto latch_ptr = std::make_shared<std::latch>(num_tasks);

        for (int64_t i = 0; i < num_tasks; ++i) {
            io_context.post([latch_ptr]() { latch_ptr->count_down(); });
        }

        latch_ptr->wait();
    };

    guard.reset();
}

TEST_CASE("io_context: post throughput - 4 workers", "[io_context][bench][throughput]") {
    svarog::io::io_context io_context;
    constexpr int64_t num_tasks = 10000;

    auto guard = svarog::execution::make_work_guard(io_context);
    std::vector<std::jthread> workers;
    for (int w = 0; w < 4; ++w) {
        workers.emplace_back([&]() { io_context.run(); });
    }

    BENCHMARK("post with 4 workers") {
        auto latch_ptr = std::make_shared<std::latch>(num_tasks);

        for (int64_t i = 0; i < num_tasks; ++i) {
            io_context.post([latch_ptr]() { latch_ptr->count_down(); });
        }

        latch_ptr->wait();
    };

    guard.reset();
}

TEST_CASE("io_context: handler latency", "[io_context][bench][latency]") {
    using namespace std::chrono;

    svarog::io::io_context io_context;
    auto guard = svarog::execution::make_work_guard(io_context);
    std::jthread worker([&]() { io_context.run(); });

    std::vector<int64_t> latencies;
    latencies.reserve(10000);

    BENCHMARK("handler execution latency") {
        auto start = high_resolution_clock::now();

        std::promise<void> done;
        auto fut = done.get_future();

        io_context.post([&, start]() {
            auto end = high_resolution_clock::now();
            latencies.push_back(duration_cast<nanoseconds>(end - start).count());
            done.set_value();
        });

        fut.wait();
    };

    guard.reset();

    // Report statistics
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        auto p50 = latencies[latencies.size() / 2];
        auto p95 = latencies[latencies.size() * 95 / 100];
        auto p99 = latencies[latencies.size() * 99 / 100];

        std::cout << "Handler Latency Statistics:\n";
        std::cout << std::format("  P50: {} ns\n", p50);
        std::cout << std::format("  P95: {} ns\n", p95);
        std::cout << std::format("  P99: {} ns\n", p99);
    }
}