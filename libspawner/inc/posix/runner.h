#ifndef _RUNNER_H_
#define _RUNNER_H_

#include <condition_variable>
#include <mutex>
#include <semaphore.h>
#include <sys/resource.h>
#include <thread>

#include "inc/base_runner.h"

#if defined(__linux__)
#include "linux_affinity.h"
#include "linux_seccomp.h"
#include "linux_procfd.h"
#endif

class runner: public base_runner {
private:
    mutex suspend_mutex;

    std::thread waitpid_thread;
    pid_t proc_pid;

    struct rusage ru;  // precise resource usage storage
    bool ru_success = false;
    bool resume_requested = false;

    //sync semaphore
    sem_t *child_sync = nullptr;

    // report pipe descriptors
    int child_report[2];
    int child_reportbuf;
#if defined(__linux__)
    linux_affinity_class affinity;
#endif
    env_vars_list_t read_environment() const;
    char **create_envp_for_process() const;
    char **create_argv_for_process() const;
    void release_argv_for_process(char **argv) const;

    // environ pointer protector, may be replaced with global lock
    mutable std::mutex envp_mtx;

    std::mutex waitpid_cond_mtx;
    std::condition_variable waitpid_cond;
    bool waitpid_ready = false;

    signal_t signal;
    int exit_code;
protected:
    void report_login();
    int change_credentials();

    unsigned long long int creation_time;
    virtual void runner_free();

    virtual void init_process(const char *cmd, char **process_argv, char **process_envp);
    virtual void create_process();
    virtual void requisites();

    virtual timeval get_user_time();
public:
    pid_t get_proc_pid();
    void run_waitpid();
    void run_monitor();
    static void *waitpid_body(void *);

    runner(const std::string &program, const options_class &options);
    virtual ~runner();
    virtual report_class get_report();
    virtual void run_process_async();
    options_class get_options() const;
    std::string get_program() const;
    virtual bool wait_for();
    bool wait_for_init(const unsigned long& interval);
    void suspend();
    void resume();
    bool is_running();
    bool start_suspended = false;
    virtual process_status_t get_process_status();
    signal_t get_signal();
    virtual unsigned long long get_time_since_create();
    static unsigned long long get_current_time();
    int get_exit_code();
};
#endif
