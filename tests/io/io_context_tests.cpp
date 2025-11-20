#include <algorithm>
#include <chrono>
#include <set>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
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

    // Use work_guard to keep workers alive
    auto guard = svarog::execution::make_work_guard(io_context);

    // Start workers FIRST
    std::vector<std::jthread> workers;
    for (int w = 0; w < num_workers; ++w) {
        workers.emplace_back([&]() { io_context.run(); });
    }

    // Give workers time to start and enter run()
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Post tasks gradually to distribute across active workers
    for (int i = 0; i < num_tasks; ++i) {
        io_context.post([&]() {
            {
                std::lock_guard lock(thread_ids_mtx);
                thread_ids.insert(std::this_thread::get_id());
            }
            completed.fetch_add(1, std::memory_order_relaxed);
        });
        // Small delay to allow thread switching
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    // Wait for all tasks to complete
    while (completed.load(std::memory_order_acquire) < num_tasks) {
        std::this_thread::yield();
    }

    // Release guard to allow workers to exit
    guard.reset();
    workers.clear();

    REQUIRE(completed.load() == num_tasks);
    REQUIRE(thread_ids.size() > 1);
}

// TEST_CASE("io_context: co_spawn coroutine", "[io_context][coroutine]") {
//     svarog::io::io_context io_context;
//     auto exec = io_context.get_executor();

//     bool resumed_on_context = false;

//     auto coro = [&]() -> svarog::execution::awaitable_task<void> {
//         REQUIRE(svarog::execution::this_coro::executor() == exec);
//         co_await io_context.schedule();
//         resumed_on_context = svarog::execution::this_coro::running_in_io_context();
//         co_return;
//     };

//     svarog::execution::co_spawn(io_context, coro(), svarog::execution::detached);

//     io_context.run();

//     REQUIRE(resumed_on_context);
// }

// TEST_CASE("io_context: coroutine cancellation", "[io_context][coroutine][cancel]") {
//     svarog::io::io_context io_context;

//     std::optional<std::error_code> completion_error;

//     auto long_running = [&]() -> svarog::execution::awaitable_task<void> {
//         for (int i = 0; i < 10; ++i) {
//             co_await io_ctx.schedule();
//             co_await svarog::execution::this_coro::check_cancellation();
//         }
//         throw std::runtime_error("boom");
//     };

//     svarog::execution::co_spawn(
//         io_context, long_running(),
//         svarog::execution::use_future([&completion_error](std::error_code ec) { completion_error = ec; }));

//     std::jthread stopper([&]() {
//         std::this_thread::sleep_for(1ms);
//         io_context.stop();
//     });

//     io_context.run();

//     REQUIRE(completion_error.has_value());
//     REQUIRE(completion_error == svarog::execution::error::operation_aborted);
// }
