#include <atomic>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <svarog/execution/work_queue.hpp>

TEST_CASE("work_queue: construction and destruction", "[work_queue][basic]") {
    SECTION("default construction") {
        svarog::execution::work_queue queue;
        REQUIRE(queue.empty());
        REQUIRE(queue.size() == 0);
    }

    SECTION("destruction with pending items") {
        {
            svarog::execution::work_queue queue;
            REQUIRE(queue.push([] { /* some work */ }));
            REQUIRE(queue.push([] { /* more work */ }));
        }  // Destructor should cleanup
        REQUIRE(true);
    }
}

TEST_CASE("work_queue: push and try_pop single-threaded", "[work_queue][basic]") {
    svarog::execution::work_queue queue;

    SECTION("try_pop on empty queue returns error") {
        auto result = queue.try_pop();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == svarog::execution::queue_error::empty);
    }

    SECTION("push and try_pop multiple items - FIFO order") {
        std::atomic<int> execution_order{0};
        int val1 = 0, val2 = 0, val3 = 0;

        REQUIRE(queue.push([&execution_order, &val1] { val1 = ++execution_order; }));
        REQUIRE(queue.push([&execution_order, &val2] { val2 = ++execution_order; }));
        REQUIRE(queue.push([&execution_order, &val3] { val3 = ++execution_order; }));

        auto r1 = queue.try_pop();
        REQUIRE(r1.has_value());
        r1.value()();  // Execute first lambda
        REQUIRE(val1 == 1);

        auto r2 = queue.try_pop();
        REQUIRE(r2.has_value());
        r2.value()();  // Execute second lambda
        REQUIRE(val2 == 2);

        auto r3 = queue.try_pop();
        REQUIRE(r3.has_value());
        r3.value()();  // Execute third lambda
        REQUIRE(val3 == 3);

        REQUIRE(queue.empty());
    }
}

TEST_CASE("work_queue: blocking pop", "[work_queue][blocking]") {
    svarog::execution::work_queue queue;

    std::atomic<bool> started{false};
    std::atomic<int> result_value{0};

    std::jthread consumer([&]() {
        started = true;
        auto result = queue.pop();  // FIXME: Implement blocking pop
        if (result) {
            result.value()();  // Execute the lambda
        }
    });

    // Wait for consumer to start
    while (!started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Short delay for consumer to block
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(result_value == 0);  // Not executed yet

    // Push item - should unblock consumer
    REQUIRE(queue.push([&result_value] { result_value = 999; }));

    consumer.join();
    REQUIRE(result_value == 999);
}

TEST_CASE("work_queue: concurrent push from multiple threads", "[work_queue][concurrency]") {
    svarog::execution::work_queue queue;
    constexpr int num_threads = 10;
    constexpr int items_per_thread = 1000;
    constexpr int total_items = num_threads * items_per_thread;

    std::atomic<int> executed_count{0};
    std::vector<std::jthread> producers;
    producers.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&queue, &executed_count]() {
            for (int i = 0; i < items_per_thread; ++i) {
                REQUIRE(queue.push([&executed_count] { executed_count++; }));
            }
        });
    }

    producers.clear();  // Implicit join

    REQUIRE(queue.size() == total_items);

    // Execute all work items
    int popped_count = 0;
    while (auto item = queue.try_pop()) {
        item.value()();  // Execute the lambda
        popped_count++;
    }

    REQUIRE(popped_count == total_items);
    REQUIRE(executed_count == total_items);
}

TEST_CASE("work_queue: producer-consumer pattern", "[work_queue][concurrency]") {
    svarog::execution::work_queue queue;
    constexpr int num_producers = 5;
    constexpr int num_consumers = 5;
    constexpr int items_per_producer = 1000;
    constexpr int total_items = num_producers * items_per_producer;

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    // Producers
    std::vector<std::jthread> producers;
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&]() {
            for (int i = 0; i < items_per_producer; ++i) {
                REQUIRE(queue.push([&consumed] { consumed++; }));
                produced++;
            }
        });
    }

    // Consumers
    std::vector<std::jthread> consumers;
    for (int c = 0; c < num_consumers; ++c) {
        consumers.emplace_back([&]() {
            while (consumed < total_items) {
                if (auto item = queue.try_pop()) {
                    item.value()();  // Execute the lambda
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    producers.clear();
    consumers.clear();

    REQUIRE(consumed == total_items);
}

TEST_CASE("work_queue: shutdown behavior", "[work_queue][shutdown]") {
    svarog::execution::work_queue queue;

    // SECTION("shutdown wakes up blocking threads") {
    //     std::atomic<bool> got_shutdown_error{ false };

    //     std::jthread consumer([&]() {
    //         auto result = queue.pop();
    //         if (!result.has_value() &&
    //             result.error() == svarog::execution::queue_error::stopped) {
    //             got_shutdown_error = true;
    //         }
    //         });

    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //     queue.stop();

    //     consumer.join();
    //     REQUIRE(got_shutdown_error);
    // }

    SECTION("shutdown with pending items allows draining") {
        std::atomic<int> val1{0}, val2{0};

        REQUIRE(queue.push([&val1] { val1 = 1; }));
        REQUIRE(queue.push([&val2] { val2 = 2; }));
        queue.stop();

        // Existing items can still be retrieved
        auto r1 = queue.try_pop();
        REQUIRE(r1.has_value());
        r1.value()();
        REQUIRE(val1 == 1);

        auto r2 = queue.try_pop();
        REQUIRE(r2.has_value());
        r2.value()();
        REQUIRE(val2 == 2);

        // Now empty and shutdown
        auto r3 = queue.try_pop();
        REQUIRE(r3.error() == svarog::execution::queue_error::stopped);
    }
}
