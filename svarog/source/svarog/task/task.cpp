#include "svarog/task/task.hpp"

namespace svarog::task
{
using coroutine_handle_t = std::coroutine_handle<TaskPromise>;

// A channel is a thread-safe primitive to communicate with worker threads -
// each worker thread has exactly one channel. Once a channel contains
// a payload it is blocked (you cannot push anymore handles onto this channel).
// The channel gets free and ready to receive another handle as soon as the
// worker thread has finished processing the current handle.
struct Channel
{
void* handle{ nullptr }; // storage for channel payload: one single handle. void means that the channel is free.
std::atomic_flag flag; // signal that the current channel is busy.

bool try_push(coroutine_handle_t& t_handle)
{
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

~Channel()
{
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


}