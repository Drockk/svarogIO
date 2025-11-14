// Based on the work of Tim Gfrerer for pal_tasks
// see <https://github.com/tgfrerer/pal_tasks>
// pal_tasks is licensed under the MIT License
#pragma once

#include <cstddef>
#include <memory>

#include <coroutine>

namespace svarog::task {
struct TaskPromise;

struct Task : std::coroutine_handle<TaskPromise> {
    using promise_type = svarog::task::TaskPromise;
};

struct suspend_task {
    constexpr bool await_ready() noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<TaskPromise> t_handle) noexcept;
    void await_resume() noexcept {};
};

struct finalize_task {
    constexpr bool await_ready() noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<TaskPromise> t_handle) noexcept;
    void await_resume() noexcept {};
};

class scheduler_impl;
class task_list_o;
class TaskList;

class Scheduler {
public:
    Scheduler(const size_t t_number_worker_threads);
    ~Scheduler();

    void wait_for_task_list(TaskList& t_taskList);

    static std::unique_ptr<Scheduler> create(const size_t t_number_worker_threads = 0);

private:
    Scheduler(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler& operator=(Scheduler&) = delete;

    scheduler_impl* m_impl{nullptr};
};

class TaskList {
public:
    TaskList(size_t t_hint_capacity = 1);
    ~TaskList();

    void add_task(Task t_task);

    friend class scheduler_impl;

private:
    TaskList(const TaskList&) = delete;
    TaskList(TaskList&&) = delete;
    TaskList& operator=(const TaskList&) = delete;
    TaskList& operator=(TaskList&) = delete;

    task_list_o* m_impl{nullptr};
};

struct TaskPromise {
    Task get_return_object() {
        return {Task::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
        return {};
    }

    finalize_task final_suspend() noexcept {
        return {};
    }
    void return_void() {
    }
    void unhandled_exception() {
    }

    scheduler_impl* scheduler = nullptr;
    task_list_o* p_task_list = nullptr;
};
}  // namespace svarog::task
