#include <chrono>
#include <set>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <latch>
#include <svarog/execution/work_guard.hpp>
#include <svarog/io/io_context.hpp>

using namespace std::chrono_literals;

TEST_CASE("io_context: post and run single handler", "[io_context][basic]") {
    svarog::io::io_context io_context;

    bool executed{false};

    io_context.post([&]() { executed = true; });

    REQUIRE_FALSE(executed);

    auto count = io_context.run_one();

    REQUIRE(executed);
    REQUIRE(count == 1);
    REQUIRE_FALSE(io_context.stopped());  // run_one() doesn't stop the context
}

TEST_CASE("io_context: multiple handlers preserve order", "[io_context][basic]") {
    svarog::io::io_context io_context;

    std::vector<int> execution_order;
    for (int i = 0; i < 10; ++i) {
        io_context.post([&, i]() { execution_order.push_back(i); });
    }

    // run() will now exit automatically when all work is done
    io_context.run();

    REQUIRE(execution_order.size() == 10);
    for (size_t i = 0; i < 10; ++i) {
        REQUIRE(execution_order[i] == static_cast<int>(i));
    }
}

TEST_CASE("io_context: dispatch vs post", "[io_context][dispatch]") {
    svarog::io::io_context io_context;

    SECTION("dispatch from inside worker thread executes immediately") {
        bool outer_executed = false;
        bool inner_executed = false;

        io_context.post([&]() {
            outer_executed = true;

            // Dispatch from inside worker thread
            io_context.dispatch([&]() { inner_executed = true; });

            // Inner should already be executed
            REQUIRE(inner_executed);
        });

        io_context.run();
        REQUIRE(outer_executed);
        REQUIRE(inner_executed);
    }
}

TEST_CASE("io_context: multiple worker threads", "[io_context][threading]") {
    constexpr int num_workers = 4;
    svarog::io::io_context io_context;

    constexpr int num_tasks = 100;
    std::atomic<int> completed{0};
    std::set<std::thread::id> thread_ids;
    std::mutex thread_ids_mtx;

    // Latch to ensure workers are running before posting tasks
    std::latch workers_ready(num_workers);

    // Use work_guard to keep workers alive
    auto guard = svarog::execution::make_work_guard(io_context);

    // Start workers FIRST
    std::vector<std::jthread> workers;
    workers.reserve(num_workers);
    for (int w = 0; w < num_workers; ++w) {
        workers.emplace_back([&]() {
            workers_ready.count_down();
            io_context.run();
        });
    }

    // Wait for all workers to start
    workers_ready.wait();

    // Post tasks that do some "work" to give other threads a chance
    for (int i = 0; i < num_tasks; ++i) {
        io_context.post([&]() {
            // Record which thread executed this task
            {
                std::lock_guard lock(thread_ids_mtx);
                thread_ids.insert(std::this_thread::get_id());
            }
            // Do a tiny bit of work to allow thread switching
            std::this_thread::yield();
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Wait for all tasks to complete
    while (completed.load(std::memory_order_acquire) < num_tasks) {
        std::this_thread::yield();
    }

    // Release guard to allow workers to exit
    guard.reset();
    workers.clear();

    // All tasks must complete
    REQUIRE(completed.load() == num_tasks);

    // At least one thread must have executed tasks
    // Note: Due to scheduling, not all workers may participate, but at least one must
    REQUIRE(thread_ids.size() >= 1);
}
