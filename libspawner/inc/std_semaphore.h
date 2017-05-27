#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include <condition_variable>
#include <mutex>

class std_semaphore
{
    std::mutex mutex_;
    std::condition_variable condition_;
    unsigned long count_ = 0;

public:
    void notify() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        ++count_;
        condition_.notify_one();
    }

    void wait() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        while(!count_) // Handle spurious wake-ups.
            condition_.wait(lock);
        --count_;
    }

    bool try_wait() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        if(count_) {
            --count_;
            return true;
        }
        return false;
    }
};

#endif //SEMAPHORE_H_
