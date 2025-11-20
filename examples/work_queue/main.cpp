#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "svarog/execution/work_queue.hpp"

using namespace svarog;
using namespace std::chrono_literals;

int main() {
    std::cout << "=== Work Queue Example ===" << std::endl;
    std::cout << "Demonstrating thread-safe MPMC queue" << std::endl;
    std::cout << std::endl;

    execution::work_queue queue;
    std::atomic<int> counter{0};

    std::cout << "--- Basic Operations ---" << std::endl;
    std::cout << "Queue size: " << queue.size() << std::endl;
    std::cout << "Queue empty: " << std::boolalpha << queue.empty() << std::endl;

    std::cout << "\n--- Pushing Work Items ---" << std::endl;

    (void)queue.push([&counter] {
        std::cout << "Task 1 executing" << std::endl;
        counter++;
    });

    (void)queue.push([&counter] {
        std::cout << "Task 2 executing" << std::endl;
        counter++;
    });

    (void)queue.push([&counter] {
        std::cout << "Task 3 executing" << std::endl;
        counter++;
    });

    std::cout << "Queue size: " << queue.size() << std::endl;
    std::cout << "Queue empty: " << queue.empty() << std::endl;

    std::cout << "\n--- Consuming with try_pop() ---" << std::endl;

    while (auto item = queue.try_pop()) {
        if (item.has_value()) {
            (*item)();
        } else if (item.error() == execution::queue_error::empty) {
            std::cout << "Queue is empty" << std::endl;
            break;
        }
    }

    std::cout << "Executed tasks: " << counter << std::endl;

    std::cout << "\n--- Multi-Producer Multi-Consumer ---" << std::endl;

    counter = 0;
    const int num_producers = 2;
    const int num_consumers = 3;
    const int tasks_per_producer = 10;

    auto producer = [&queue, tasks_per_producer](int id) {
        for (int i = 0; i < tasks_per_producer; ++i) {
            (void)queue.push([id, i] {
                std::cout << "Producer " << id << " - Task " << i << std::endl;
                std::this_thread::sleep_for(10ms);
            });
        }
        std::cout << "Producer " << id << " finished" << std::endl;
    };

    auto consumer = [&queue, &counter](int id) {
        while (true) {
            auto item = queue.try_pop();
            if (item.has_value()) {
                (*item)();
                counter++;
            } else if (item.error() == execution::queue_error::stopped) {
                std::cout << "Consumer " << id << " detected stop" << std::endl;
                break;
            } else {
                std::this_thread::yield();
            }
        }
    };

    std::vector<std::jthread> producers;
    std::vector<std::jthread> consumers;

    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer, i);
    }

    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(consumer, i);
    }

    for (auto& p : producers) {
        p.join();
    }

    std::this_thread::sleep_for(200ms);

    std::cout << "\n--- Stopping Queue ---" << std::endl;
    queue.stop();

    for (auto& c : consumers) {
        c.join();
    }

    std::cout << "Total tasks executed: " << counter << std::endl;
    std::cout << "Queue stopped: " << std::boolalpha << queue.stopped() << std::endl;

    std::cout << "\n=== Example Complete ===" << std::endl;
    return 0;
}
