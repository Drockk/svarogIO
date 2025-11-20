#include <chrono>
#include <iostream>
#include <thread>

#include "svarog/execution/awaitable_task.hpp"
#include "svarog/execution/co_spawn.hpp"
#include "svarog/execution/work_guard.hpp"
#include "svarog/io/io_context.hpp"

using namespace svarog;
using namespace std::chrono_literals;

auto fetch_data(io::io_context& ctx, int id) -> execution::awaitable_task<int> {
    std::cout << "Fetching data for ID: " << id << std::endl;

    co_await ctx.schedule();

    std::this_thread::sleep_for(100ms);

    std::cout << "Data fetched for ID: " << id << std::endl;
    co_return id * 10;
}

auto process_item(io::io_context& ctx, int item_id) -> execution::awaitable_task<void> {
    std::cout << "Processing item " << item_id << std::endl;

    int data = co_await fetch_data(ctx, item_id);

    co_await ctx.schedule();

    std::cout << "Processed item " << item_id << " with result: " << data << std::endl;

    co_return;
}

auto main_coroutine(io::io_context& ctx) -> execution::awaitable_task<void> {
    std::cout << "Starting async workflow..." << std::endl;

    for (int i = 1; i <= 3; ++i) {
        co_await process_item(ctx, i);
    }

    std::cout << "All items processed!" << std::endl;
    co_return;
}

int main() {
    std::cout << "=== Simple Coroutine Example ===" << std::endl;
    std::cout << "Demonstrating C++20 coroutines with io_context" << std::endl;
    std::cout << std::endl;

    io::io_context ctx;

    execution::co_spawn(ctx, main_coroutine(ctx), execution::detached);

    auto guard = execution::make_work_guard(ctx);

    std::jthread worker([&ctx] { ctx.run(); });

    std::this_thread::sleep_for(2s);

    guard.reset();
    worker.join();

    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;

    return 0;
}
