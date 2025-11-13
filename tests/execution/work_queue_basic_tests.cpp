#include <atomic>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <svarog/execution/work_queue.hpp>

TEST_CASE("work_queue basic operations") {
    svarog::execution::work_queue queue;

    SECTION("push and try pop") {
        bool called = false;
        REQUIRE(queue.push([&] { called = true; }));

        auto result = queue.try_pop();
        REQUIRE(result.has_value());
        (*result)();
        REQUIRE(called);
    }

    SECTION("empty queue") {
        REQUIRE(queue.empty());
        auto result = queue.try_pop();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == svarog::execution::queue_error::empty);
    }

    SECTION("stopped queue") {
        queue.stop();
        REQUIRE(queue.stopped());
        REQUIRE_FALSE(queue.push([] {}));

        auto result = queue.try_pop();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == svarog::execution::queue_error::stopped);
    }
}

TEST_CASE("work_queue concurrent push") {
    svarog::execution::work_queue queue;
    std::atomic<int> counter{0};

    std::vector<std::thread> threads;
    for (auto i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            for (auto j = 0; j < 100; ++j) {
                std::ignore = queue.push([&] { ++counter; });
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    while (auto result = queue.try_pop()) {
        if (result) {
            (*result)();
        }
    }

    REQUIRE(counter.load() == 400);
}
