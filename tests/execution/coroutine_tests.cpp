#include <atomic>
#include <chrono>
#include <thread>

#include "svarog/execution/awaitable_task.hpp"
#include "svarog/execution/co_spawn.hpp"
#include "svarog/execution/work_guard.hpp"
#include "svarog/io/io_context.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace svarog;
using namespace std::chrono_literals;

TEST_CASE("awaitable_task: basic coroutine", "[coroutine][awaitable_task]") {
    auto simple_coro = []() -> execution::awaitable_task<int> { co_return 42; };

    auto task = simple_coro();
    REQUIRE(task.valid());

    auto result_coro = [](execution::awaitable_task<int> t) -> execution::awaitable_task<int> {
        int value = co_await t;
        co_return value;
    };

    auto result_task = result_coro(std::move(task));

    if (result_task.handle()) {
        result_task.handle().resume();
        if (result_task.handle().done()) {
            int result = result_task.await_resume();
            REQUIRE(result == 42);
        }
    }
}

TEST_CASE("awaitable_task: void return type", "[coroutine][awaitable_task]") {
    bool executed = false;

    auto void_coro = [&]() -> execution::awaitable_task<void> {
        executed = true;
        co_return;
    };

    auto task = void_coro();
    REQUIRE(task.valid());
    REQUIRE_FALSE(executed);

    if (task.handle()) {
        task.handle().resume();
        REQUIRE(executed);
    }
}

TEST_CASE("io_context: schedule awaiter", "[io_context][coroutine][schedule]") {
    io::io_context ctx;
    std::atomic<bool> resumed{false};
    std::atomic<bool> coroutine_started{false};

    auto coro = [&]() -> execution::awaitable_task<void> {
        coroutine_started = true;
        co_await ctx.schedule();
        resumed = true;
        co_return;
    };

    auto task = coro();
    REQUIRE(task.valid());

    // awaitable_task suspends at initial_suspend, need to resume manually
    if (task.handle() && !task.handle().done()) {
        task.handle().resume();
    }

    REQUIRE(coroutine_started);
    REQUIRE_FALSE(resumed);

    ctx.run_one();
    REQUIRE(resumed);
}

TEST_CASE("io_context: co_spawn with detached", "[io_context][coroutine][co_spawn]") {
    io::io_context ctx;
    std::atomic<bool> coro_ran{false};
    std::atomic<int> step{0};

    auto coro = [&]() -> execution::awaitable_task<void> {
        step = 1;
        co_await ctx.schedule();
        step = 2;
        co_await ctx.schedule();
        step = 3;
        coro_ran = true;
        co_return;
    };

    execution::co_spawn(ctx, coro(), execution::detached);

    REQUIRE(step == 0);
    REQUIRE_FALSE(coro_ran);

    auto guard = execution::make_work_guard(ctx);
    std::jthread worker([&] { ctx.run(); });

    std::this_thread::sleep_for(100ms);

    guard.reset();
    worker.join();

    REQUIRE(coro_ran);
    REQUIRE(step == 3);
}

TEST_CASE("io_context: multiple coroutines", "[io_context][coroutine][multiple]") {
    io::io_context ctx;
    std::atomic<int> counter{0};

    auto worker_coro = [&](int id) -> execution::awaitable_task<void> {
        for (int i = 0; i < 5; ++i) {
            co_await ctx.schedule();
            counter++;
        }
        co_return;
    };

    for (int i = 0; i < 4; ++i) {
        execution::co_spawn(ctx, worker_coro(i), execution::detached);
    }

    auto guard = execution::make_work_guard(ctx);
    std::jthread runner([&] { ctx.run(); });

    while (counter.load() < 20) {
        std::this_thread::sleep_for(10ms);
    }

    guard.reset();
    runner.join();

    REQUIRE(counter == 20);
}

TEST_CASE("io_context: coroutine with return value", "[io_context][coroutine][return]") {
    io::io_context ctx;
    std::atomic<int> result{0};

    auto compute = [&]() -> execution::awaitable_task<int> {
        co_await ctx.schedule();
        int value = 42;
        co_await ctx.schedule();
        value *= 2;
        co_return value;
    };

    auto consumer = [&]() -> execution::awaitable_task<void> {
        int val = co_await compute();
        result = val;
        co_return;
    };

    execution::co_spawn(ctx, consumer(), execution::detached);

    auto guard = execution::make_work_guard(ctx);
    std::jthread runner([&] { ctx.run(); });

    while (result.load() == 0) {
        std::this_thread::sleep_for(10ms);
    }

    guard.reset();
    runner.join();

    REQUIRE(result == 84);
}

TEST_CASE("io_context: nested coroutine calls", "[io_context][coroutine][nested]") {
    io::io_context ctx;
    std::atomic<int> depth{0};

    auto level3 = [&]() -> execution::awaitable_task<int> {
        co_await ctx.schedule();
        depth = 3;
        co_return 100;
    };

    auto level2 = [&]() -> execution::awaitable_task<int> {
        co_await ctx.schedule();
        depth = 2;
        int val = co_await level3();
        co_return val + 10;
    };

    auto level1 = [&]() -> execution::awaitable_task<void> {
        depth = 1;
        int val = co_await level2();
        REQUIRE(val == 110);
        REQUIRE(depth == 3);
        co_return;
    };

    execution::co_spawn(ctx, level1(), execution::detached);

    auto guard = execution::make_work_guard(ctx);
    std::jthread runner([&] { ctx.run(); });

    while (depth.load() < 3) {
        std::this_thread::sleep_for(10ms);
    }

    guard.reset();
    runner.join();

    REQUIRE(depth == 3);
}
