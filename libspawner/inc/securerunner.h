#pragma once

#include <list>
#include <string>

#include "runner.h"

class secure_runner: public runner
{
protected:
    handle_t hIOCP;
    handle_t hJob;
    restrictions_class restrictions;
    thread_t check_thread, completition;
    std::list<handle_t> threads;

    terminate_reason_t terminate_reason;

    virtual bool create_restrictions();
    virtual void apply_restrictions();
    virtual void create_process();

    static thread_return_t process_completition_proc(thread_param_t param);
    static thread_return_t check_limits_proc(thread_param_t param);

    void dump_threads(bool suspend);
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

    //extended functions
    bool is_running();

    void suspend();
    void resume();
};
