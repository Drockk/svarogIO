#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "svarog/execution/thread_pool.hpp"
#include "svarog/execution/work_guard.hpp"

using namespace svarog;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Thread Pool Example ===" << std::endl;
    std::cout << "Demonstrating RAII thread pool management" << std::endl;
    std::cout << std::endl;

    std::cout << "--- Example 1: Basic Thread Pool ---" << std::endl;
    {
        execution::thread_pool pool(4);
        auto guard = execution::make_work_guard(pool.context());

        std::cout << "Thread count: " << pool.thread_count() << std::endl;
        std::cout << "Stopped: " << std::boolalpha << pool.stopped() << std::endl;

        auto counter = std::make_shared<std::atomic<int>>(0);

        for (int i = 0; i < 10; ++i) {
            pool.post([counter] { (*counter)++; });
        }

        std::this_thread::sleep_for(50ms);
        std::cout << "Tasks executed: " << *counter << std::endl;

        guard.reset();  // Allow pool to exit
    }

    std::cout << "\n--- Example 2: CPU-bound Work Distribution ---" << std::endl;
    {
        execution::thread_pool pool(4);
        auto guard = execution::make_work_guard(pool.context());

        std::cout << "Distributing work across 4 threads" << std::endl;

        auto sum = std::make_shared<std::atomic<long long>>(0);
        const int num_tasks = 20;

        for (int i = 0; i < num_tasks; ++i) {
            pool.post([sum, i] {
                long long value = i * 1000;
                (*sum) += value;
            });
        }

        std::this_thread::sleep_for(100ms);

        std::cout << "Final sum: " << *sum << std::endl;
        std::cout << "Expected: " << (19 * 20 / 2 * 1000) << std::endl;

        guard.reset();
    }

    std::cout << "\n--- Example 3: Access to Underlying Context ---" << std::endl;
    {
        execution::thread_pool pool(2);
        auto guard = execution::make_work_guard(pool.context());

        auto& ctx = pool.context();
        auto executor = pool.get_executor();

        ctx.post([] { std::cout << "Posted directly to context" << std::endl; });

        executor.execute([] { std::cout << "Executed via executor" << std::endl; });

        pool.post([] { std::cout << "Posted via pool" << std::endl; });

        std::this_thread::sleep_for(50ms);
        guard.reset();
    }

    std::cout << "\n--- Example 4: Producer-Consumer Pattern ---" << std::endl;
    {
        execution::thread_pool pool(4);
        auto guard = execution::make_work_guard(pool.context());

        auto produced = std::make_shared<std::atomic<int>>(0);
        auto consumed = std::make_shared<std::atomic<int>>(0);

        auto producer = [&pool, produced, consumed](int count) {
            for (int i = 0; i < count; ++i) {
                pool.post([consumed] { (*consumed)++; });
                (*produced)++;
            }
        };

        std::jthread p1([&] { producer(5); });
        std::jthread p2([&] { producer(5); });

        p1.join();
        p2.join();

        std::this_thread::sleep_for(50ms);

        std::cout << "Produced: " << *produced << ", Consumed: " << *consumed << std::endl;
        guard.reset();
    }

    std::cout << "\n--- Example 5: Graceful Shutdown ---" << std::endl;
    {
        execution::thread_pool pool(2);
        auto guard = execution::make_work_guard(pool.context());

        for (int i = 0; i < 5; ++i) {
            pool.post([i] {
                std::cout << "Task " << i << " running" << std::endl;
                std::this_thread::sleep_for(50ms);
            });
        }

        std::this_thread::sleep_for(100ms);

        std::cout << "Initiating graceful shutdown..." << std::endl;
        guard.reset();

        std::cout << "Waiting for tasks to complete..." << std::endl;
        pool.stop();
        pool.wait();

        std::cout << "All tasks completed" << std::endl;
    }

    std::cout << "\n--- Example 6: RAII Automatic Cleanup ---" << std::endl;
    {
        std::cout << "Creating pool..." << std::endl;
        {
            execution::thread_pool pool(2);

            pool.post([] { std::cout << "Task in RAII pool" << std::endl; });

            std::this_thread::sleep_for(50ms);

            std::cout << "Leaving scope (pool destructor will stop and join)..." << std::endl;
        }
        std::cout << "Pool destroyed" << std::endl;
    }

    std::cout << "\n=== Example Complete ===" << std::endl;
    return 0;
}
