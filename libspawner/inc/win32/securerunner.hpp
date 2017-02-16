#pragma once

#include <string>
#include <atomic>
#include <functional>

#include "runner.h"

class secure_runner: public runner
{
protected:
    handle_t hIOCP;
    handle_t hJob;
    restrictions_class start_restrictions;
    restrictions_class restrictions;
    thread_t check_thread;
    terminate_reason_t terminate_reason;
    std::atomic<bool> prolong_time_limits_{false};
    LONGLONG base_time_processor_ = 0;
    unsigned long long base_time_user_ = 0;

    virtual bool create_restrictions();
    virtual void apply_restrictions();
    virtual void create_process();

    static thread_return_t check_limits_proc(thread_param_t param);

    virtual void free();
    virtual void wait();
    virtual void requisites();
public:
    secure_runner(const std::string &program, const options_class &options,
        const restrictions_class &restrictions);
    virtual ~secure_runner();

    terminate_reason_t get_terminate_reason();

    restrictions_class get_restrictions() const;
    process_status_t get_process_status();
    virtual report_class get_report();
    void prolong_time_limits();
    bool force_stop = false;
    std::function<void()> on_terminate;
};
