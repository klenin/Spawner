#ifndef _SPAWNER_RUNNER_H_
#define _SPAWNER_RUNNER_H_

#include <string>
#include <map>// <-- for pipes only
#include "platform.h"
#include "pipes.h"
#include "status.h"
#include "report.h"
#include "options.h"

class runner
{
private:
    DWORD process_creation_flags;
    startupinfo_t si;
    options_class options;
    std::string program;
protected:
    std::map<pipes_t, pipe_class*> pipes;
    process_info_t process_info;
    process_status_t process_status;
    bool running;
    report_class report;
    bool init_process(char *cmd, const char *wd);
    bool init_process_with_logon(char *cmd, const char *wd);
    virtual void create_process();
    virtual void free();
    virtual void wait();
public:
    runner(const std::string &program, const options_class &options);
    ~runner();
    unsigned long get_exit_code();
    virtual process_status_t get_process_status();
    exception_t get_exception();
    unsigned long get_id();
    std::string get_program() const;
    options_class get_options() const;
    virtual report_class get_report();

    virtual void run_process();
    void wait(const unsigned long &interval);
    virtual void safe_release();
    void set_pipe(const pipes_t &pipe_type, pipe_class *pipe_object);
};

#endif//_SPAWNER_RUNNER_H_
