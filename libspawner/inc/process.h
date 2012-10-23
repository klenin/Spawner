#ifndef _SPAWNER_PROCESS_H_
#define _SPAWNER_PROCESS_H_

#include <list>
#include <string>
#include <time.h>//dd
#include "restrictions.h"
#include "platform.h"
#include "pipes.h"
#include "status.h"
#include "report.h"
#include "options.h"

class process_class
{
private:
    DWORD process_creation_flags;
    startupinfo_t si;
    options_class options;
    std::string program;
protected:
    pipe_class std_input, std_output, std_error;
    process_info_t process_info;
    process_status_t process_status;
    bool init_process(char *cmd, const char *wd);
    bool init_process_with_logon(char *cmd, const char *wd);
    void create_process();
    virtual void free()
    {
    }
public:
    process_class(const std::string &program, const options_class &options);
    ~process_class();
    unsigned long get_exit_code();
    process_status_t get_process_status();
    exception_t get_exception();
    handle_t get_handle();
    unsigned long get_id();
    std::string get_program() const;
    options_class get_options() const;
    void run_process();
    void wait(const unsigned long &interval);
};

class process_wrapper
{
protected:
    handle_t hIOCP;
    handle_t hJob;
    process_class process;
    restrictions_class restrictions;
    thread_t check_thread, completition;
    std::list<handle_t> threads;

    bool running;

    process_status_t process_status;
    terminate_reason_t terminate_reason;
    report_class report;

    void apply_restrictions();

    static thread_return_t process_completition_proc(thread_param_t param);
    static thread_return_t check_limits_proc(thread_param_t param);

    void dump_threads(bool suspend);
    void wait();
public:
    process_wrapper(const std::string &program, const options_class &options, const restrictions_class &restrictions);
    ~process_wrapper();
    void run_process();
    void run_async();

    terminate_reason_t get_terminate_reason();
    report_class get_report();
    process_status_t get_process_status();

    //extended functions
    bool is_running();

    void suspend();
    void resume();
};

#endif//_SPAWNER_PROCESS_H_
