#ifndef _SP_MUTEX_HPP_
#define _SP_MUTEX_HPP_

#include <atomic>
#include <windows.h>

#include "inc/error.hpp"
#include "platform.hpp"

class mutex_c {
public:
    mutex_c() {
        if (instance_count_ == 0) {
            if (handle != INVALID_HANDLE_VALUE) {
                // TODO: report invalid state
            }
            handle = CreateMutex(NULL, FALSE, NULL);
            if (handle == NULL) {
                PANIC(get_win_last_error_string());
            }
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

    void lock() {
        DWORD wait_result = WaitForSingleObject(handle, INFINITE);
        switch (wait_result) {
        case WAIT_OBJECT_0:
        case WAIT_TIMEOUT:
            break;
        default:
            PANIC(get_win_last_error_string());
        }
    }

    bool is_locked() {
        DWORD wait_result = WaitForSingleObject(handle, 0);
        switch (wait_result) {
        case WAIT_OBJECT_0:
            return false;
        case WAIT_TIMEOUT:
            return true;
        default:
            PANIC(get_win_last_error_string());
            return false; // Silence warning
        }
    }

    void unlock() {
        if (!ReleaseMutex(handle)) {
            PANIC(get_win_last_error_string());
        }
    }

private:
    mutable std::atomic<unsigned> instance_count_{0};
    HANDLE handle = INVALID_HANDLE_VALUE;
};

#endif // _SP_MUTEX_HPP_
