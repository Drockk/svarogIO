#include <chrono>
#include <iostream>
#include <thread>

#include "svarog/execution/work_guard.hpp"
#include "svarog/io/io_context.hpp"

using namespace svarog;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Work Guard Example ===" << std::endl;
    std::cout << "Demonstrating RAII lifetime management" << std::endl;
    std::cout << std::endl;

    std::cout << "--- Example 1: Without Work Guard ---" << std::endl;
    {
        io::io_context ctx;

        ctx.post([] { std::cout << "Task 1 executed" << std::endl; });

        std::cout << "Starting run() - will exit after task completes" << std::endl;
        ctx.run();
        std::cout << "run() exited" << std::endl;
    }

    std::cout << "\n--- Example 2: With Work Guard (Manual Control) ---" << std::endl;
    {
        io::io_context ctx;
        auto guard = execution::make_work_guard(ctx);

        ctx.post([] { std::cout << "Task 1 executed" << std::endl; });

        std::jthread worker([&ctx] {
            std::cout << "Worker thread: Starting run()" << std::endl;
            ctx.run();
            std::cout << "Worker thread: run() exited" << std::endl;
        });

        std::this_thread::sleep_for(100ms);

        ctx.post([] { std::cout << "Task 2 executed (posted after delay)" << std::endl; });

        std::this_thread::sleep_for(100ms);

        std::cout << "Main thread: Releasing guard..." << std::endl;
        guard.reset();

        worker.join();
    }

    std::cout << "\n--- Example 3: Guard Prevents Premature Exit ---" << std::endl;
    {
        io::io_context ctx;
        auto guard = execution::make_work_guard(ctx);

        std::jthread worker([&ctx] {
            std::cout << "Worker: run() started (waiting for work)" << std::endl;
            ctx.run();
            std::cout << "Worker: run() completed" << std::endl;
        });

        std::this_thread::sleep_for(100ms);
        std::cout << "Main: Posting delayed work..." << std::endl;

        ctx.post([] { std::cout << "Delayed task executed!" << std::endl; });

        std::this_thread::sleep_for(100ms);

        std::cout << "Main: Releasing guard to allow exit" << std::endl;
        guard.reset();

        worker.join();
    }

    std::cout << "\n--- Example 4: Move Semantics ---" << std::endl;
    {
        io::io_context ctx;

        auto guard1 = execution::make_work_guard(ctx);
        std::cout << "Guard 1 owns work: " << std::boolalpha << guard1.owns_work() << std::endl;

        auto guard2 = std::move(guard1);
        std::cout << "After move:" << std::endl;
        std::cout << "  Guard 1 owns work: " << guard1.owns_work() << std::endl;
        std::cout << "  Guard 2 owns work: " << guard2.owns_work() << std::endl;

        guard2.reset();
        std::cout << "After reset:" << std::endl;
        std::cout << "  Guard 2 owns work: " << guard2.owns_work() << std::endl;
    }

    std::cout << "\n--- Example 5: Multiple Guards ---" << std::endl;
    {
        io::io_context ctx;

        auto guard1 = execution::make_work_guard(ctx);
        auto guard2 = execution::make_work_guard(ctx);

        std::cout << "Two guards active" << std::endl;

        std::jthread worker([&ctx] { ctx.run(); });

        std::this_thread::sleep_for(100ms);

        std::cout << "Releasing guard 1..." << std::endl;
        guard1.reset();

        std::this_thread::sleep_for(100ms);
        std::cout << "run() still active (guard 2 active)" << std::endl;

        std::cout << "Releasing guard 2..." << std::endl;
        guard2.reset();

        worker.join();
        std::cout << "run() exited after all guards released" << std::endl;
    }

    std::cout << "\n=== Example Complete ===" << std::endl;
    return 0;
}
