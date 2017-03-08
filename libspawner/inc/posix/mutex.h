#ifndef _POSIX_MUTEX_H_
#define _POSIX_MUTEX_H_

#include <atomic>
#include <mutex>

#include "inc/error.h"

// TODO: all this code shall be reviewed since right now it just provides pipes/buffer stubs compilation

class mutex_c {
public:
    mutex_c() {
        std::atomic_fetch_add_explicit(&instance_count_, 1u, std::memory_order_relaxed);
    }

    ~mutex_c() {
        if (std::atomic_fetch_sub_explicit(&instance_count_, 1u, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            handle.unlock();
        }
    }

    void lock() {
        handle.lock();
    }

    bool is_locked() {
        std::unique_lock<std::recursive_mutex> lock(handle);
        if (lock.owns_lock())
            return true;

        return false;
    }

    void unlock() {
        handle.unlock();
    }

private:
    mutable std::atomic<unsigned> instance_count_{0};
    std::recursive_mutex handle;
};
#endif // _POSIX_MUTEX_H_
