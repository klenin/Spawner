#ifndef _SPAWNER_PROCESS_H_
#define _SPAWNER_PROCESS_H_

#include <list>
#include <vector>//dd
#include <string>
#include <time.h>//dd
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"
#include "status.h"
#include "report.h"
#include "options.h"

using namespace std;

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
    virtual void init_process(char *cmd, const char *wd);
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
    list<handle_t> threads;

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
/*
class CProcess
{
protected:
    CProcess(){};

    string application;
    string worging_directory;
    list<string> arguments;
    //arguments
    //working directory

    process_info_t process_info;
    startupinfo_t si;

    thread_t thread, check, completition, debug_thread;
    handle_t process_initiated;
    handle_t hIOCP;
    handle_t hJob;

    list<handle_t> threads;

    CPipe std_input, std_output, std_error;

    CRestrictions restrictions;
    COptions options;
    bool running;

    process_status_t process_status;
    terminate_reason_t terminate_reason;
    CReport report;

    static thread_return_t debug_thread_proc(thread_param_t param);
    static thread_return_t process_completition(thread_param_t param);
    static thread_return_t check_limits_proc(thread_param_t param);

    //to fix
    void createProcess();
    void setRestrictions();
    void wait();
    void finish();
public:
	CProcess(const string &file);
    void prepare();
    int run();
    bool set_terminate_reason(const terminate_reason_t &reason);
    process_id get_process_id();
	int Run();
    void RunAsync();
    //TODO implement Wait function

	~CProcess();

    istringstream &stdoutput();
    istringstream &stderror();

    unsigned long get_exit_code();
    void set_restrictions(const CRestrictions &Restrictions);
    void set_options(const COptions &Options);
    void suspend();
    void resume();
    void dump_threads(bool suspend = false);
    process_status_t get_process_status();
    bool is_running();
    exception_t get_exception();
    terminate_reason_t get_terminate_reason();
    CReport get_report();
    void Finish();
    bool Wait(const unsigned long &ms_time);
};
*/
#endif//_SPAWNER_PROCESS_H_
