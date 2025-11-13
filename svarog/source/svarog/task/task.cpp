#include "svarog/task/task.hpp"

#include <iostream>
#include <vector>

#include <assert.h>

#include "svarog/task/lockfree_ring_buffer.hpp"

namespace svarog::task {
using coroutine_handle_t = std::coroutine_handle<TaskPromise>;

// A channel is a thread-safe primitive to communicate with worker threads -
// each worker thread has exactly one channel. Once a channel contains
// a payload it is blocked (you cannot push anymore handles onto this channel).
// The channel gets free and ready to receive another handle as soon as the
// worker thread has finished processing the current handle.
struct Channel {
    void* handle{nullptr};  // storage for channel payload: one single handle. void means that the channel is free.
    std::atomic_flag flag;  // signal that the current channel is busy.

    bool try_push(coroutine_handle_t& t_handle) {
        if (flag.test_and_set()) {
            // if the current channel was already flagged
            // we cannot add anymore work.
            return false;
        }

        // --------| invariant: current channel is available now

        handle = t_handle.address();

        // If there is a thread blocked on this operation, we
        // unblock it here.
        flag.notify_one();

        return true;
    }

    ~Channel() {
        // Once the channel accepts a coroutine handle, it becomes the
        // owner of the handle. If there are any leftover valid handles
        // that we own when this object dips into ovlivion, we must clean
        // them up first.
        if (handle) {
            Task::from_address(handle).destroy();
        }

        handle = nullptr;
    }
};

class task_list_o {
public:
    std::atomic_flag block_flag;  // flag used to signal that dependent tasks have completed

    task_list_o(size_t t_capacity_hint = 32) : m_tasks(t_capacity_hint), m_num_tasks(0) {
    }

    ~task_list_o() {
        // If there are any tasks left on the task list, we must destroy them, as we own them.
        void* task{nullptr};
        while ((task = m_tasks.try_pop())) {
            Task::from_address(task).destroy();
        }
    }

    inline void push_task(coroutine_handle_t const& t_handle) {
        m_tasks.push(t_handle.address());
    }

    // Get the next task if possible, if there is no next task,
    // return an empty coroutine handle.
    // An empty coroutine handle will compare true to nullptr
    inline coroutine_handle_t pop_task() {
        return Task::from_address(m_tasks.try_pop());
    }

    // Return the number of tasks which are both in flight and waiting
    // Note this is not the same as tasks.size() as any tasks which are being
    // processed and are in flight will not show up on the task list.
    // num_tasks gets decremented only if a task was fully completed.
    inline size_t get_tasks_count() {
        return m_num_tasks;
    }

    // Add a new task to the task list - only allowed in setup phase,
    // where only one thread has access to the task list.
    void add_task(coroutine_handle_t& t_handle) {
        t_handle.promise().p_task_list = this;
        m_tasks.unsafe_initial_dynamic_push(t_handle.address());
        m_num_tasks++;
    }

    void tag_all_tasks_with_scheduler(scheduler_impl* p_scheduler) {
        m_tasks.unsafe_for_each(
            [](void* c, void* p_scheduler) {
                coroutine_handle_t::from_address(c).promise().scheduler = static_cast<scheduler_impl*>(p_scheduler);
            },
            p_scheduler);
    }

    void decrement_task_count() {
        size_t num_flags = --m_num_tasks;
        if (num_flags == 0) {
            block_flag.clear(std::memory_order_release);
            block_flag.notify_one();  // unblock us on block flag.
        }
    }

private:
    lockfree_ring_buffer_t m_tasks;
    std::atomic_size_t m_num_tasks;  // number of tasks, only gets decremented if taks has been removed
};

TaskList::TaskList(size_t t_hint_capacity) : m_impl(new task_list_o(t_hint_capacity)) {
}

TaskList::~TaskList() {
    // In case that this task list was deleted already, m_impl will
    // be nullptr, which means that this delete operator is a no-op.
    // otherwise (in case a tasklist has not been used and needs to
    // be cleaned up), this will perform the cleanup for us.
    delete m_impl;
}

void TaskList::add_task(Task t_task) {
    assert(m_impl != nullptr && "task list must be valid. Was this task list already used?");
    m_impl->add_task(t_task);
}

class scheduler_impl {
public:
    std::vector<Channel*> m_channels;  // non-owning - channels are owned by their threads
    std::vector<std::jthread> m_threads;

    scheduler_impl(size_t t_num_worker_threads = 0) {
        // Reserve memory so that we can take addresses for channel,
        // and don't have to worry about iterator validity

        m_channels.reserve(t_num_worker_threads);
        m_threads.reserve(t_num_worker_threads);

        // NOTE THAT BY DEFAULT WE DON'T HAVE ANY WORKER THREADS
        for (int i = 0; i != t_num_worker_threads; i++) {
            m_channels.emplace_back(new Channel());
            m_threads.emplace_back(
                //
                // Thread worker implementation
                //
                [](std::stop_token t_stop_token, Channel* t_channel) {
                    while (!t_stop_token.stop_requested()) {
                        if (t_channel->handle) {
                            coroutine_handle_t::from_address(t_channel->handle).resume();
                            t_channel->handle = nullptr;
                            // signal that we are ready to receive new tasks
                            t_channel->flag.clear(std::memory_order::release);
                            continue;
                        }

                        // Wait for flag to be set
                        //
                        // The flag is set on any of the following:
                        //   * A new job has been placed in the channel
                        //   * The current task list is empty.
                        t_channel->flag.wait(false, std::memory_order::acquire);
                    }

                    // Channel is owned by the thread - when the thread falls out of scope
                    // that means that the channel gets deleted, too.
                    delete t_channel;
                },
                m_channels.back());
        }
    }

    ~scheduler_impl() {
        // We must unblock any threads which are currently waiting on a flag signal for more work
        // as there is no more work coming, we must artificially signal the flag so that these
        // worker threads can resume to completion.
        for (auto* channel : m_channels) {
            if (channel) {
                channel->flag
                    .test_and_set();  // Set flag so that if there is a worker blocked on this flag, it may proceed.
                channel->flag
                    .notify_one();  // Notify the worker thread (if any worker thread is waiting) that the flag has
                                    // flipped. without notify, waiters will not be notified that the flag has flipped.
            }
        }
        // We must wait until all the threads have been joined.
        // Deleting a jthread object implicitly stops (sets the stop_token) and joins.
        m_threads.clear();
    }

    // Execute all tasks in the task list, then invalidate the task list object
    void wait_for_task_list(TaskList& t_task_list) {
        if (t_task_list.m_impl == nullptr) {
            assert(false &&
                   "Task list must have been freshly initialised. Has this task list been waited for already?");
            return;
        }

        // --------| Invariant: TaskList is valid

        // Execute tasks in this task list until there are no more tasks left
        // to execute.

        // Before we start executing tasks we must take ownership of them
        // by tagging them so that they know which scheduler they belong to.
        t_task_list.m_impl->tag_all_tasks_with_scheduler(this);

        // Distribute work, as long as there is work to distribute
        while (t_task_list.m_impl->get_tasks_count()) {
            // ----------| Invariant: There are Tasks in this Task List which have not yet completed
            coroutine_handle_t c = (t_task_list.m_impl)->pop_task();

            if (c == nullptr) {
                // We could not fetch a task from the task list - this means
                // that there are tasks in-progress that we must wait for.

                if (t_task_list.m_impl->block_flag.test_and_set(std::memory_order::acq_rel)) {
                    std::cout << "blocking thread " << std::this_thread::get_id() << " on [" << t_task_list.m_impl
                              << "]" << std::endl;
                    // Wait for the flag to be set - this is the case if any of these happen:
                    //    * the scheduler is destroyed
                    //    * the last task of the task list has completed, and the task list is now empty.
                    t_task_list.m_impl->block_flag.wait(true, std::memory_order::acquire);
                    std::cout << "resuming thread " << std::this_thread::get_id() << " on [" << t_task_list.m_impl
                              << "]" << std::endl;
                } else {
                    std::cout << "spinning thread " << std::this_thread::get_id() << " on [" << t_task_list.m_impl
                              << "]" << std::endl;
                }

                continue;
            }

            // ----------| Invariant: current coroutine is valid
            // Find a free channel. if there is, then place this handle in the channel,
            // which means that it will be executed on the worker thread associated
            // with this channel.

            if (move_task_to_worker_thread(c)) {
                // Pushing consumes the coroutine handle - that is, it becomes owned by the channel
                // who owns it for the worker thread.
                //
                // If we made it in here, the handle was successfully offloaded to a worker thread.
                //
                // The worker thread must now execute the payload, and the task will decrement the
                // counter for the current TaskList upon completion.
                continue;
            }

            // --------| Invariant: All worker threads are busy - or there are no worker threads: we must execute on
            // this thread
            c();
        }

        // Once all tasks have been complete, release task list
        delete t_task_list.m_impl;     // Free task list impl
        t_task_list.m_impl = nullptr;  // Signal to any future users that this task list has been used already
    }

private:
    bool move_task_to_worker_thread(coroutine_handle_t& t_handle) {
        // Iterate over all channels. If we can place the coroutine
        // on a channel, do so.
        for (auto& ch : m_channels) {
            if (true == ch->try_push(t_handle)) {
                return true;
            }
        }

        return false;
    }
};

Scheduler::Scheduler(const size_t t_number_worker_thread) : m_impl(new scheduler_impl(t_number_worker_thread)) {
}

void Scheduler::wait_for_task_list(TaskList& t_task_list) {
    m_impl->wait_for_task_list(t_task_list);
}

std::unique_ptr<Scheduler> Scheduler::create(const size_t t_number_worker_threads) {
    return std::make_unique<Scheduler>(t_number_worker_threads);
}

Scheduler::~Scheduler() {
    delete m_impl;
}

void suspend_task::await_suspend(std::coroutine_handle<TaskPromise> h) noexcept {
    // ----------| Invariant: At this point the coroutine pointed to by h
    // has been fully suspended. This is guaranteed by the c++ standard.

    auto& promise = h.promise();

    auto& task_list = promise.p_task_list;

    // Put the current coroutine to the back of the scheduler queue
    // as it has been fully suspended at this point.

    task_list->push_task(promise.get_return_object());

    {
        // We must unblock/awake the scheduling thread each time we suspend
        // a coroutine so that the scheduling worker may pick up work again,
        // in case it had been put to sleep earlier.
        promise.p_task_list->block_flag.clear(std::memory_order_release);
        promise.p_task_list->block_flag.notify_one();  // wake up worker just in case
    }

    {
        // --- Eager Workers ---
        //
        // Eagerly try to fetch & execute the next task from the front of the
        // scheduler queue -
        // We do this so that multiple threads can share the
        // scheduling workload.
        //
        // But we can also disable that, so that there is only one thread
        // that does the scheduling, and removing elements from the
        // queue.

        coroutine_handle_t c = task_list->pop_task();

        if (c) {
            assert(!c.done() && "task must not be done");
            c();
        }
    }

    // Note: Once we drop off here, control will return to where the resume()
    // command that brought us here was issued.
}

// ----------------------------------------------------------------------

void finalize_task::await_suspend(std::coroutine_handle<TaskPromise> h) noexcept {
    // This is the last time that this coroutine will be awakened
    // we do not suspend it anymore after this
    h.promise().p_task_list->decrement_task_count();
    h.destroy();  // are we allowed to destroy here?
}
}  // namespace svarog::task