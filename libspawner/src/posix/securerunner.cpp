#include "securerunner.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rlimit.h"
#include "error.h"

#define load_ratios_max_size 20

secure_runner::secure_runner(const std::string &program,
    const options_class &options, const restrictions_class &restrictions):
    runner(program, options),
    start_restrictions(restrictions),
    restrictions(restrictions),
    terminate_reason(terminate_reason_not_terminated),
    max_load_ratio_index(-1),
    prev_consumed(0),
    prev_elapsed(0)
{
}

secure_runner::~secure_runner()
{
}

void secure_runner::create_process() {
    runner::create_process();
}


void secure_runner::init_process(const char *cmd_toexec, char **process_argv, char **process_envp) {
    create_restrictions();
    runner::init_process(cmd_toexec, process_argv, process_envp);
}

bool secure_runner::create_restrictions() {
    // XXX check return values and report to the parent
    if (check_restriction(restriction_memory_limit))
        // linux&cygwin both supports Address Space rlimit
#if defined(__linux__)
        impose_rlimit(RLIMIT_AS, 2*get_restriction(restriction_memory_limit));
#elif defined(__CYGWIN__)
        impose_rlimit(RLIMIT_AS, get_restriction(restriction_memory_limit));
#else
        // openbsd and os x does not, switch to Resident Size
        // note that they both will just shrink process rss if memory is tight
        impose_rlimit(RLIMIT_RSS, get_restriction(restriction_memory_limit));
#endif

#if !defined(__linux__) // linux version has a procfs judge
    if (check_restriction(restriction_processor_time_limit))
        impose_rlimit(RLIMIT_CPU, get_restriction(restriction_processor_time_limit) / 1000000);
#endif
    if (check_restriction(restriction_security_limit)) {
        impose_rlimit(RLIMIT_CORE, 0);
#if defined(__linux__)
        if (seccomp_probe_filter())
            exit(EXIT_FAILURE);
        else
            seccomp_setup_filter();
#endif // XXX warning for non linuxes
    }

    return true;
}


void secure_runner::requisites() {
    creation_time = get_current_time();

    monitor_thread = std::thread(check_limits_proc, reinterpret_cast<void*>(this));
    // wait for monitor thread
    std::unique_lock<std::mutex> lock(monitor_cond_mtx);
    while (!monitor_ready)
        monitor_cond.wait(lock);

    runner::requisites();
}

timeval secure_runner::get_user_time() {
    timeval a;
    a.tv_sec = (int)proc_consumed;
    a.tv_usec = (int)(1000000 * (proc_consumed - (int)proc_consumed));

    return a;
}

report_class secure_runner::get_report() {
    report.terminate_reason = get_terminate_reason();
    // non linux OSes will get ru_maxrss from getrusage
#if defined(__linux__)
    report.write_transfer_count = proc.discovered ? proc.write_bytes : 0;
    report.peak_memory_used = proc.discovered ? proc.vss_max : 0;
#endif
    return runner::get_report();
}

bool secure_runner::wait_for() {
    runner::wait_for();
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }

    return true;
}

terminate_reason_t secure_runner::get_terminate_reason() {
    // TODO: rework all thats rye

    switch (get_signal()) {
    case signal_signal_no: break;
    case SIGABRT:
    case SIGFPE:
    case SIGQUIT:
    case SIGSEGV:
    case SIGSYS:
    case SIGTRAP:
    case SIGALRM:
    case SIGILL:
    case SIGTERM:
    case SIGXFSZ:
    case SIGBUS: {// abnormal exit means signal (exception in windows)
        terminate_reason = terminate_reason_abnormal_exit_process;
        break;
    }
    case SIGKILL: // if process was killed by the monitor thread, take into
                  // account terminate reason.
        if (terminate_reason == terminate_reason_user_time_limit
            || terminate_reason == terminate_reason_write_limit
            || terminate_reason == terminate_reason_load_ratio_limit
            || terminate_reason == terminate_reason_time_limit
            || terminate_reason == terminate_reason_memory_limit)
            break;
        else { // manually killed by a human or hard rlimit
            terminate_reason = terminate_reason_abnormal_exit_process;
            break;
        }
    case SIGXCPU: {
        terminate_reason = terminate_reason_user_time_limit;
        break;
    }
    default:
        terminate_reason = terminate_reason_abnormal_exit_process;
    }

    return terminate_reason;
}

restrictions_class secure_runner::get_restrictions() const {
    return restrictions;
}

restriction_t secure_runner::get_restriction(const restriction_kind_t &restriction) const {
    return restrictions.get_restriction(restriction);
}

bool secure_runner::check_restriction(const restriction_kind_t &restriction) const {
    return restrictions.check_restriction(restriction);
}

process_status_t secure_runner::get_process_status() {
    if (process_status == process_finished_terminated || process_status == process_suspended)
        return process_status;
    return runner::get_process_status();
}

void secure_runner::prolong_time_limits() {
    prolong_time_limits_ = true;
}

void secure_runner::runner_free() {
    runner::runner_free();
}

void *secure_runner::check_limits_proc(void *monitor_param) {
    // add SIGSTOP and getitimer() support
    pid_t proc_pid;

    struct timespec req;

    secure_runner *self = reinterpret_cast<secure_runner*>(monitor_param);

    proc_pid = self->get_proc_pid();

#if defined(__linux__)
    unsigned long long current_time;
    double restriction; // TODO -- get rid of FPU calculations.

    int tick_res = sysconf(_SC_CLK_TCK);
    long ticks_elapsed = 0;
    bool tick_detected = false;
    long tick_to_micros = 1000000 / tick_res;

    if(!self->proc.probe_pid(proc_pid)) {
        kill(proc_pid, SIGKILL);
        PANIC("procfs entry not created");
    }
    self->proc.fill_all();
#endif

    req.tv_sec=0;
    req.tv_nsec=0;

    // tv_sec computation may be useless
    if (self->options.monitorInterval >= 1000000)
        req.tv_sec = (self->options.monitorInterval / 1000000);
    // convert ms to ns
    if (self->options.monitorInterval % 1000000)
        req.tv_nsec = (self->options.monitorInterval % 1000000) * 1000;

    // report back to main thread
    {
        std::lock_guard<std::mutex> lock(self->monitor_cond_mtx);
        self->monitor_ready = true;
    }
    self->monitor_cond.notify_one();

    // Main loop..
    while (self->running) {
#if defined(__linux__)
        if(!self->proc.fill_all())
            break;

        if (self->check_restriction(restriction_write_limit) &&
            (self->proc.write_bytes > self->get_restriction(restriction_write_limit))) {
            // stop process and take a death mask
            kill(proc_pid, SIGSTOP);
            self->proc.fill_all();
            // terminate
            kill(proc_pid, SIGKILL);
            self->terminate_reason = terminate_reason_write_limit;
            self->process_status = process_finished_terminated;
            break;
        }

        // The common virtual tick detection code (only for appropriate judges).
        // -- Procfs utime entry is updated (only) when virtual "user space"
        //    (The Linux kernel can be idle-tickless or even fully tickless,
        //    so it provides virtual ticks for user space) "tick" ticks.
        // -- Do not perform calculations too often (only after "tick" ticks).
        if (self->check_restriction(restriction_idle_time_limit) ||
            self->check_restriction(restriction_load_ratio) ||
            self->check_restriction(restriction_processor_time_limit)
        ) {
            current_time = self->get_time_since_create() / 10;
            if (tick_detected = (current_time / tick_to_micros > ticks_elapsed)) {
                ticks_elapsed = current_time / tick_to_micros;
            }
        }

        if (self->check_restriction(restriction_memory_limit) &&
            self->proc.vss_max > self->get_restriction(restriction_memory_limit)
        ) {
            self->terminate_reason = terminate_reason_memory_limit;
            self->process_status = process_finished_terminated;
            break;
        }

        // The load ratio judge
        // -- Calculate the ratio only when clock "ticks" at least first 5 times
        //    (controlled process starts to consume CPU ticks after some time,
        //    at least on my 1.4GHz Core2Solo), uncomment printfs and check with
        //    "./sp --out /dev/null /bin/yes"
#define TICK_THRESHOLD 5

        self->proc_consumed = static_cast<double>(self->proc.stat_utime) / tick_res;
        double wclk_elapsed = static_cast<double>(current_time) / 1000000;
        double load_ratio = (self->proc_consumed - self->prev_consumed) / (wclk_elapsed - self->prev_elapsed);

        if (wclk_elapsed - self->prev_elapsed > 0.2) {
            self->prev_consumed = self->proc_consumed;
            self->prev_elapsed = wclk_elapsed;
        }

        if (self->check_restriction(restriction_load_ratio) && tick_detected &&
            self->check_restriction(restriction_idle_time_limit) &&
            ticks_elapsed >= TICK_THRESHOLD
        ) {
            int idle_limit = self->get_restriction(restriction_idle_time_limit) / 1000000;
            int step = idle_limit * self->options.monitorInterval / 10 / load_ratios_max_size;
            if (ticks_elapsed % step == 0 && self->last_tick != ticks_elapsed) {
                // printf("consumed: %g (procfs value %lu) ", proc_consumed, self->proc.stat_utime);
                restriction = static_cast<double>(self->get_restriction(restriction_load_ratio)) / 10000;
                // printf("load_ratio: %g, restriction: %g\n", load_ratio, restriction);
                bool can_use_load_ratios = self->load_ratios.size() == load_ratios_max_size;
                if (can_use_load_ratios) {
                    self->load_ratios.pop_back();
                    if (self->max_load_ratio_index == load_ratios_max_size - 1) {
                        self->max_load_ratio_index = 0;
                        for (int i = 1; i < load_ratios_max_size - 1; ++i) {
                            if (self->load_ratios[self->max_load_ratio_index] < self->load_ratios[i]) {
                                self->max_load_ratio_index = i;
                            }
                        }
                    }
                }
                self->load_ratios.push_front(load_ratio);
                ++self->max_load_ratio_index;
                if (self->load_ratios[self->max_load_ratio_index] <= load_ratio) {
                    self->max_load_ratio_index = 0;
                }
                if (can_use_load_ratios &&
                    self->load_ratios[self->max_load_ratio_index] < restriction
                ) {
                    kill(proc_pid, SIGSTOP);
                    self->proc.fill_all();
                    kill(proc_pid, SIGKILL);
                    self->terminate_reason = terminate_reason_load_ratio_limit;
                    self->process_status = process_finished_terminated;
                    break;
                }
            }
            self->last_tick = ticks_elapsed;
        }

        // precise cpu usage judge
        if (self->check_restriction(restriction_processor_time_limit) && tick_detected) {
            double restriction = static_cast<double>(self->get_restriction(restriction_processor_time_limit)) / 1000000;
            // printf("time limit: %g, utime: %lu, consumed: %g\n",
            //    (double)restriction, self->proc.stat_utime, proc_consumed);
            if (self->proc_consumed >= restriction) {
                // stopped process has no chance to handle SIGXCPU
                //kill(proc_pid, SIGSTOP);
                self->proc.fill_all();
                // SIGXCPU can be ignored
                kill(proc_pid, SIGXCPU);
                // wait some time for signal delivery
                usleep(static_cast<useconds_t>(tick_to_micros));
                kill(proc_pid, SIGKILL);
                self->terminate_reason = terminate_reason_time_limit;
                self->process_status = process_finished_terminated;
            }
        }

#endif
        if (self->check_restriction(restriction_user_time_limit) &&
            (self->get_time_since_create() / 10) > self->get_restriction(restriction_user_time_limit)) {
#if defined(__linux__)
            self->proc.fill_all();
#endif
            kill(proc_pid, SIGXCPU);
#if defined(__linux__)
            usleep(static_cast<useconds_t>(tick_to_micros));
#else
            usleep(100 * 1000);
#endif
            kill(proc_pid, SIGKILL);
            self->terminate_reason = terminate_reason_user_time_limit;
            self->process_status = process_finished_terminated;
        }

        nanosleep(&req, nullptr);
    }

    return nullptr;
}
