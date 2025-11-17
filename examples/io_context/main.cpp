#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "svarog/execution/work_guard.hpp"
#include "svarog/io/io_context.hpp"

using namespace svarog;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== IO Context Example ===" << std::endl;
    std::cout << "Demonstrating event loop and async execution" << std::endl;
    std::cout << std::endl;

    std::cout << "--- Example 1: Basic post() and run() ---" << std::endl;
    {
        io::io_context ctx;

        ctx.post([] { std::cout << "Task 1 executed" << std::endl; });

        ctx.post([] { std::cout << "Task 2 executed" << std::endl; });

        ctx.post([] { std::cout << "Task 3 executed" << std::endl; });

        std::cout << "Calling run()..." << std::endl;
        ctx.run();
        std::cout << "run() completed" << std::endl;
    }

    std::cout << "\n--- Example 2: dispatch() vs post() ---" << std::endl;
    {
        io::io_context ctx;

        ctx.post([&ctx] {
            std::cout << "In handler, calling dispatch()..." << std::endl;

            ctx.dispatch([] { std::cout << "  dispatch() executes immediately (same thread)" << std::endl; });

            std::cout << "After dispatch()" << std::endl;

            ctx.post([] { std::cout << "  post() defers execution" << std::endl; });

            std::cout << "After post()" << std::endl;
        });

        ctx.run();
    }

    std::cout << "\n--- Example 3: Multi-threaded Execution ---" << std::endl;
    {
        io::io_context ctx;
        std::atomic<int> counter{0};
        const int num_workers = 4;
        const int num_tasks = 20;

        for (int i = 0; i < num_tasks; ++i) {
            ctx.post([&counter, i] {
                std::cout << "Task " << i << " on thread " << std::this_thread::get_id() << std::endl;
                counter++;
                std::this_thread::sleep_for(10ms);
            });
        }

        std::vector<std::jthread> workers;
        for (int i = 0; i < num_workers; ++i) {
            workers.emplace_back([&ctx, i] {
                std::cout << "Worker " << i << " started" << std::endl;
                ctx.run();
                std::cout << "Worker " << i << " finished" << std::endl;
            });
        }

        for (auto& w : workers) {
            w.join();
        }

        std::cout << "Tasks executed: " << counter << std::endl;
    }

    std::cout << "\n--- Example 4: run_one() for Step Execution ---" << std::endl;
    {
        io::io_context ctx;

        ctx.post([] { std::cout << "Task A" << std::endl; });
        ctx.post([] { std::cout << "Task B" << std::endl; });
        ctx.post([] { std::cout << "Task C" << std::endl; });

        std::cout << "Executing one task at a time:" << std::endl;
        while (ctx.run_one() > 0) {
            std::cout << "  (processing next task)" << std::endl;
        }
    }

    std::cout << "\n--- Example 5: stop() and restart() ---" << std::endl;
    {
        io::io_context ctx;
        auto guard = execution::make_work_guard(ctx);

        std::jthread worker([&ctx] {
            std::cout << "Worker: run() started" << std::endl;
            ctx.run();
            std::cout << "Worker: run() stopped" << std::endl;
        });

        std::this_thread::sleep_for(100ms);

        ctx.post([] { std::cout << "Task before stop" << std::endl; });
        std::this_thread::sleep_for(50ms);

        std::cout << "Main: Calling stop()..." << std::endl;
        ctx.stop();
        guard.reset();

        worker.join();

        std::cout << "Main: Context stopped, restarting..." << std::endl;
        ctx.restart();

        ctx.post([] { std::cout << "Task after restart" << std::endl; });
        ctx.run();
    }

    std::cout << "\n--- Example 6: Executor Pattern ---" << std::endl;
    {
        io::io_context ctx;

        auto executor = ctx.get_executor();

        executor.execute([] { std::cout << "Executed via executor" << std::endl; });

        auto executor2 = ctx.get_executor();
        std::cout << "Executors equal: " << std::boolalpha << (executor == executor2) << std::endl;

        ctx.run();
    }

    std::cout << "\n--- Example 7: Long-running Event Loop ---" << std::endl;
    {
        io::io_context ctx;
        auto guard = execution::make_work_guard(ctx);
        std::atomic<bool> running{true};

        std::jthread worker([&ctx] { ctx.run(); });

        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(100ms);
            ctx.post([i] { std::cout << "Periodic task " << i << std::endl; });
        }

        std::cout << "Shutting down..." << std::endl;
        guard.reset();
        worker.join();
    }

    std::cout << "\n=== Example Complete ===" << std::endl;
    return 0;
}
