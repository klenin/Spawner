#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "securerunner.h"

#include "rlimit.h"

secure_runner::secure_runner(const std::string &program, 
    const options_class &options, const restrictions_class &restrictions)
    : runner(program, options)
    , start_restrictions(restrictions)
    , restrictions(restrictions)
    , terminate_reason(terminate_reason_not_terminated)
{
    pthread_mutex_init(&monitor_lock, NULL);
    pthread_cond_init(&monitor_cond, NULL);
}

secure_runner::~secure_runner()
{
    pthread_mutex_destroy(&monitor_lock);
    pthread_cond_destroy(&monitor_cond);
}


void secure_runner::create_process() {
    runner::create_process();
}

void secure_runner::prepare_stdio() {
    int in, out, err;
    // multiple stdout/error files are not supported

    // no panics here since we are "child"
    if (options.stdinput.size() == 1)
        if((access(options.stdinput[0].c_str(), R_OK) != -1)) {
            in = open(options.stdinput[0].c_str(), O_RDONLY | O_NOFOLLOW);
            if (in == -1)
                exit(EXIT_FAILURE);
            else {
                close(STDIN_FILENO);
                dup2(in, STDIN_FILENO);
            }
        }

    if (options.stdoutput.size() == 1) {
        out = open(options.stdoutput[0].c_str(),
            O_WRONLY | O_CREAT | O_NOFOLLOW,
            S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
        if (out == -1)
            exit(EXIT_FAILURE);
        else {
            close(STDOUT_FILENO);
            dup2(out, STDOUT_FILENO);
        }
    }

    // try to merge stderr and stdout
    if (options.stderror.size() == 1) {
        if(options.stderror[0] == options.stdoutput[0]) {
            close(STDERR_FILENO);
            dup2(STDOUT_FILENO, STDERR_FILENO);
        }
        else {
            err = open(options.stderror[0].c_str(),
                O_WRONLY | O_CREAT | O_NOFOLLOW,
                S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
            if (err == -1)
                exit(EXIT_FAILURE);
            else {
                close(STDERR_FILENO);
                dup2(err, STDERR_FILENO);
            }
        }
    }
}


void secure_runner::init_process(const char *cmd_toexec, char **process_argv, char **process_envp) {
    prepare_stdio();
    create_restrictions();
    runner::init_process(cmd_toexec, process_argv, process_envp);
}

bool secure_runner::create_restrictions() {
    // XXX check return values and report to the parent
    if (restrictions.get_restriction(restriction_memory_limit) !=
        restriction_no_limit)
        // linux&cygwin both supports Address Space rlimit
#if defined(__linux__) || defined(__CYGWIN__)
        impose_rlimit(RLIMIT_AS, restrictions.get_restriction(restriction_memory_limit));
#else
        // openbsd and os x does not, switch to Resident Size
        // note that they both will just shrink process rss if memory is tight
        impose_rlimit(RLIMIT_RSS, restrictions.get_restriction(restriction_memory_limit));
#endif

#if !defined(__linux__) // linux version has a procfs judge
    if (restrictions.get_restriction(restriction_processor_time_limit) != restriction_no_limit)
        impose_rlimit(RLIMIT_CPU, restrictions.get_restriction(restriction_processor_time_limit) / 1000000);
#endif

    if (restrictions.get_restriction(restriction_security_limit) != restriction_no_limit) {
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

    // wait till monitor thread starts
    pthread_mutex_lock(&monitor_lock);
    pthread_create(&monitor_thread, NULL, check_limits_proc, (void *)this);
    while (!monitor_ready)
        pthread_cond_wait(&monitor_cond, &monitor_lock);
    pthread_mutex_unlock(&monitor_lock);

    runner::requisites();
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

bool secure_runner::wait_for()
{
    runner::wait_for();
    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);
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
            || terminate_reason == terminate_reason_time_limit)
            break;
        else { // manually killed by a human or hard rlimit 
            terminate_reason
                = terminate_reason_abnormal_exit_process;
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



restrictions_class secure_runner::get_restrictions() const
{
    return restrictions;
}

process_status_t secure_runner::get_process_status()
{
    if (process_status == process_finished_terminated
        || process_status == process_suspended)
        return process_status;

    return runner::get_process_status();
}


void secure_runner::prolong_time_limits() {
    prolong_time_limits_ = true;
}

void secure_runner::runner_free()
{
    runner::runner_free();
}

void *secure_runner::check_limits_proc(void *monitor_param) {
    // add SIGSTOP and getitimer() support
    pid_t proc_pid;
    int poparg;

    struct timespec req;
    
    secure_runner *self = (secure_runner *)monitor_param;

    proc_pid = self->get_proc_pid();

#if defined(__linux__)
    int tick_res;
    long int ticks_elapsed, tick_to_micros;
    unsigned long long int current_time;
    bool tick_detected;
    double proc_consumed, restriction; // TODO -- get rid of FPU calculations.

    tick_res = sysconf(_SC_CLK_TCK);
    ticks_elapsed = 0;
    tick_detected = false;
    tick_to_micros = 1000000 / tick_res;

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

    // report that monitor thread has been started
    pthread_mutex_lock(&self->monitor_lock);
    self->monitor_ready = true;
    pthread_mutex_unlock(&self->monitor_lock);
    pthread_cond_signal(&self->monitor_cond);

    // Main loop..
    // Linux version will break upon procfs entry removal.
    // All others will be cancelled upon pthread_cancel().
    pthread_cleanup_push(monitor_cleanup, NULL); 
    while (1) {
        pthread_testcancel();
#if defined(__linux__)
        if(!self->proc.fill_all())
            break;
        if (self->restrictions.get_restriction(restriction_write_limit) != restriction_no_limit && (self->proc.write_bytes > self->restrictions.get_restriction(restriction_write_limit))) {
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
        if ((self->restrictions.get_restriction(restriction_load_ratio)
            != restriction_no_limit) ||
            (self->restrictions.get_restriction(restriction_processor_time_limit)           != restriction_no_limit)) {
            current_time = self->get_time_since_create() / 10;
            if ((current_time / tick_to_micros) > ticks_elapsed) {
                proc_consumed = (double) self->proc.stat_utime / tick_res;
                ticks_elapsed = current_time / tick_to_micros;
                tick_detected = true;
            } else
                tick_detected = false;
        }

        // The load ratio judge
        // -- Calculate the ratio only when clock "ticks" at least first 5 times
        //    (controlled process starts to consume CPU ticks after some time,
        //    at least on my 1.4GHz Core2Solo), uncomment printfs and check with
        //    "./sp --out /dev/null /bin/yes"
#define TICK_THRESHOLD 5
        if (self->restrictions.get_restriction(restriction_load_ratio)
            != restriction_no_limit) {
            // printf("ticks: %lu\n", ticks_elapsed);
            if (tick_detected && (ticks_elapsed >= TICK_THRESHOLD)) {
                double wclk_elapsed = (double) current_time / 1000000;
                double load_ratio = proc_consumed / wclk_elapsed;
                // printf("consumed: %g (procfs value %lu) ", proc_consumed, self->proc.stat_utime);
                restriction = (double)self->restrictions.get_restriction(restriction_load_ratio) / 10000;
                // printf("load_ratio: %g, restriction: %g\n", load_ratio, restriction);
                if (load_ratio < restriction) {
                    kill(proc_pid, SIGSTOP);
                    self->proc.fill_all();
                    kill(proc_pid, SIGKILL);
                    self->terminate_reason = terminate_reason_load_ratio_limit;
                    self->process_status = process_finished_terminated;
                    break;
                }
            }
        }

        // precise cpu usage judge
        if (self->restrictions.get_restriction(restriction_processor_time_limit)            != restriction_no_limit) {
            if (tick_detected) {
                double restriction = (double)self->restrictions.get_restriction(restriction_processor_time_limit) / 1000000;
                // printf("time limit: %g, utime: %lu, consumed: %g\n",
                //    (double)restriction, self->proc.stat_utime, proc_consumed);
                if (proc_consumed >= restriction) {
                    // stopped process has no chance to handle SIGXCPU
                    //kill(proc_pid, SIGSTOP);
                    self->proc.fill_all();
                    // SIGXCPU can be ignored
                    kill(proc_pid, SIGXCPU);
                    // wait some time for signal delivery
                    usleep((useconds_t)tick_to_micros);
                    kill(proc_pid, SIGKILL);
                    self->terminate_reason = terminate_reason_time_limit;
                    self->process_status = process_finished_terminated;
                }
            }
        }

        // TODO: vmemory judge with correct termination reason

#endif
        if (( self->get_time_since_create() / 10 ) >
            self->restrictions.get_restriction(restriction_user_time_limit)) {
                self->proc.fill_all();
                kill(proc_pid, SIGXCPU);
                usleep((useconds_t)tick_to_micros);
                kill(proc_pid, SIGKILL);
                self->terminate_reason = terminate_reason_user_time_limit;
                self->process_status = process_finished_terminated;
        }

        nanosleep(&req, nullptr);
    }

    pthread_cleanup_pop(poparg);
    pthread_exit(NULL);
}


void secure_runner::monitor_cleanup(void *cleanup_param) {
    // do nothing
}
