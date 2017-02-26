#ifndef _SP_MUTEX_HPP_
#define _SP_MUTEX_HPP_

#include <atomic>
#include <pthread.h>

#include "inc/error.hpp"

class mutex_c {
public:
    mutex_c() {
        if (instance_count_ == 0) {
            pthread_mutex_init(&handle, NULL);
        }
        std::atomic_fetch_add_explicit(&instance_count_, 1u, std::memory_order_relaxed);
    }

    ~mutex_c() {
        if (std::atomic_fetch_sub_explicit(&instance_count_, 1u, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            //unlock prior to destroy
            unlock();
            pthread_mutex_destroy(&handle);
        }
    }

    void lock() {
        if (pthread_mutex_lock(&handle)) {
            PANIC("failed to lock mutex");
        }
    }

    // there are no easy way to check pthread mutex state
    bool is_locked() {
        if (pthread_mutex_trylock(&handle)) {
            return true;
        } else {
            // mutex is locked, unlock it back
            unlock();
            return false; 
        }
    }

    void unlock() {
        if (pthread_mutex_unlock(&handle)) {
            PANIC("failed to unlock mutex");
        }
    }

private:
    mutable std::atomic<unsigned> instance_count_{0};
    pthread_mutex_t handle;
};

#endif // _SP_MUTEX_HPP_
