#ifndef _SECURE_RUNNER_H_
#define _SECURE_RUNNER_H_

#include <atomic>
#include <functional>
#include <queue>

#include "runner.h"

#if defined(__linux__)
#include "linux_procfs.h"
#include "linux_seccomp.h"
#endif

class secure_runner: public runner
{
private:
#if defined(__linux__)
    procfs_class proc; // rough resource usage storage
#endif
    double proc_consumed;

    std::thread proc_thread, monitor_thread;
    void run_monitor();

    void prepare_stdio();

    std::mutex monitor_cond_mtx;
    std::condition_variable monitor_cond;
    bool monitor_ready = false;

protected:
    restrictions_class start_restrictions;
    restrictions_class restrictions;
    double prev_consumed, prev_elapsed;
    int last_tick, max_load_ratio_index;
    std::deque<double> load_ratios;
    terminate_reason_t terminate_reason;
    std::atomic<bool> prolong_time_limits_{false};
    virtual bool create_restrictions();
    virtual void init_process(const char *cmd_toexec, char **process_argv, char **process_envp);
    virtual void create_process();

    void check_limits_proc();

    virtual void runner_free();
    virtual void requisites();

    virtual void wait() override;

    virtual timeval get_user_time() override;
public:
    secure_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions);
    virtual ~secure_runner();

    terminate_reason_t get_terminate_reason();

    restrictions_class get_restrictions() const;
    restriction_t get_restriction(const restriction_kind_t &restriction) const;
    bool check_restriction(const restriction_kind_t &restriction) const;
    process_status_t get_process_status();
    virtual report_class get_report();
    void prolong_time_limits();
    bool force_stop = false;
    std::function<void()> on_terminate;
};
#endif // _SECURE_RUNNER_H_
