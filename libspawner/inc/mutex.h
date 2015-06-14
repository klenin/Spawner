#pragma once

#include <atomic>
#include <windows.h>

class mutex_c {
public:
    mutex_c() {
        if (instance_count_ == 0) {
            if (handle != INVALID_HANDLE_VALUE) {
                // TODO: report invalid state
            }
            handle = CreateMutex(NULL, FALSE, NULL);
        }
        std::atomic_fetch_add_explicit(&instance_count_, 1u, std::memory_order_relaxed);
    }

    ~mutex_c() {
        if (handle == INVALID_HANDLE_VALUE) {
            // TODO: report invalid state
        }
        if (std::atomic_fetch_sub_explicit(&instance_count_, 1u, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    }

    bool lock() {
        return WaitForSingleObject(handle, INFINITE) == WAIT_OBJECT_0;
    }

    void unlock() {
        ReleaseMutex(handle);
    }

private:
    mutable std::atomic<unsigned> instance_count_ = 0;
    HANDLE handle = INVALID_HANDLE_VALUE;
};
