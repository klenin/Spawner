#include <iostream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>

#include "inc/error.h"

#include "runner.h"

runner::runner (const std::string &program, const options_class &options)
    : base_runner(program, options)
{
}

runner::~runner()
{
}

process_status_t runner::get_process_status() {
    return process_status;
}

runner::env_vars_list_t runner::read_environment() const
{
    env_vars_list_t vars;
    extern char **environ;
    char *const *envp = environ;

    while (*envp) {
        std::string envStr(*envp++);
        int pos = envStr.find("=");
        vars.push_back(make_pair(envStr.substr(0, pos), envStr.substr(pos + 1)));
    }

    return vars;
}

char **runner::create_envp_for_process() const
{
    extern char **environ;
    char **original;
    char **result;

    auto curr_vars = read_environment();
    original = environ;

    // environ variable is a shared resource
    envp_mtx.lock();
#ifdef __MACH__
    // TODO: Find a real solution for OS X
#else
    environ = nullptr; //dont call clearenv() since it releases memory
#endif
    if (options.environmentMode == "user-default") {
        PANIC("user-default mode is not supported");
        //setenv("SPAWNER_VERSION", "POSIX", 1);
        } else if (options.environmentMode == "clear")
        for (auto i = curr_vars.cbegin(); i != curr_vars.cend(); ++i)
            // use unsetenv() or leave environ pointer with NULL
            setenv(i->first.c_str(), "" , 1);
        else if (options.environmentMode == "inherit")
        for (auto i = curr_vars.cbegin(); i != curr_vars.cend(); ++i)
            setenv(i->first.c_str(), i->second.c_str(), 1);

        for (auto i = options.environmentVars.cbegin(); i != options.environmentVars.cend(); ++i)
            setenv(i->first.c_str(), i->second.c_str(), 1);


    // restore original environ pointer
    result = environ;
    environ = original;
    envp_mtx.unlock();

    return result;
}

char **runner::create_argv_for_process() const
{
    char **result, *argv_buff;
    int i;
    size_t argv_count, argv_len, argv_pos;

    // prepare argv_len - total lens of progname + all arguments + all
    // terminating \0
    argv_len = program.size() + 1;
    argv_count = options.get_arguments_count();
    for (i = 0; i < argv_count; i++)
        argv_len += options.get_argument(i).size() + 1;

    // TODO: check return values from mallocs and count max len
    // progname/argv count

    // Array of pointers [argv_count + final terminating NULL ptr]
    // Plus ptr to progname
    result = (char **)malloc((argv_count + 2) * sizeof(char *));

    argv_buff = (char *)malloc(argv_len);

    // First element of argv is progname.
    // Copy progname and all arguments with terminating nulls to buffer.
    result[0] = argv_buff;
    strncpy(argv_buff, program.c_str(), program.size() + 1);
    argv_pos = program.size() + 1;

    // arguments
    for (i = 0; i < argv_count; i++) {
        strncpy(argv_buff + argv_pos, options.get_argument(i).c_str(), options.get_argument(i).size() + 1);
        result[i + 1] = argv_buff + argv_pos;
        argv_pos += options.get_argument(i).size() + 1;
    }

    // Last element of a result array is terminating NULL ptr
    result[argv_count + 1] = nullptr;

    return result;
}

void runner::release_argv_for_process(char **argv) const
{
    if ((argv != nullptr) && (argv[0] != nullptr)) {
        free(argv[0]);
        free(argv);
    }
}

pid_t runner::get_proc_pid() {
    return proc_pid;
}
std::string runner::get_program() const {
    return program;
}

unsigned long long runner::get_time_since_create() {
    unsigned long long current = runner::get_current_time();
    if (current < creation_time)
        current = creation_time;

    return current - creation_time;
}

unsigned long long runner::get_current_time()
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        // XXX shut off process
        PANIC("get_current_time(): failed clock_gettime()");
    }

    // 100ns resolution
    return (unsigned long long)((ts.tv_sec * 10000000) + (ts.tv_nsec / 100));
}

void runner::init_process(const char *cmd_toexec, char **process_argv, char **process_envp) {
    // try to change credentials, exit on fail.
    // TODO - return value over pipe
    if (options.login != "") {
        if (change_credentials())
            exit(EXIT_FAILURE);
    }
    sem_wait(child_sync); //syncronize with parent
    execve(cmd_toexec, process_argv, process_envp);
}

signal_t runner::get_signal() {
    if ((get_process_status() == process_finished_abnormally)
        || (get_process_status() == process_finished_terminated)) {
        return (signal);
    } else
        return signal_signal_no;
}

void *runner::waitpid_body(void *waitpid_param) {
    int status;
    runner *self = (runner *)waitpid_param;

    // report back to main thread
    {
      std::lock_guard<std::mutex> lock(self->waitpid_cond_mtx);
      self->waitpid_ready = true;
    }
    self->waitpid_cond.notify_one();

    self->signal = signal_signal_no;
    self->exit_code = 0;
    do {
        pid_t w = waitpid(self->proc_pid, &status,
            WUNTRACED | WCONTINUED);
        if (WIFSIGNALED(status)) {
            // "signalled" means abnormal/terminate
#ifdef WCOREDUMP
            self->process_status = process_finished_abnormally;
#else
            self->process_status = process_finished_terminated;
#endif

            self->signal = (signal_t)WTERMSIG(status);
        } else if (WIFEXITED(status)) {
            self->process_status = process_finished_normal;
            self->exit_code = WEXITSTATUS(status);
        } else if (WIFSTOPPED(status))
            self->process_status = process_suspended;
        else if (WIFCONTINUED(status))
            self->process_status = process_still_active;
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    // get wall_clock time
    self->report.user_time = self->get_time_since_create() / 10;

    if (getrusage(RUSAGE_CHILDREN, &self->ru) != -1) // TODO: not working with multiple runs
        self->ru_success = true;

#ifdef __linux__
    timeval t = self->get_user_time();
    self->ru.ru_utime.tv_sec = t.tv_sec;
    self->ru.ru_utime.tv_usec = t.tv_usec;
#endif

    for (auto& stream : self->streams) {
        stream.second->finalize();
    }

    return(NULL);
}

void runner::run_process_async() {
    running_async = true;
    create_process();
}

bool runner::wait_for()
{
    if (waitpid_thread.joinable()) {
        waitpid_thread.join();
    }
    running = false;
    return true;
}

void runner::requisites() {
#if defined(__linux__)
    affinity.set(proc_pid);
#endif
    // wait till waitpid body completely starts
    waitpid_thread = std::thread(waitpid_body, (void *)this);
    std::unique_lock<std::mutex> lock(waitpid_cond_mtx);
    while (!waitpid_ready)
        waitpid_cond.wait(lock);
}

timeval runner::get_user_time() {
}

void runner::report_login() {
    uid_t u, eu;

    struct passwd *pwdp, pwd;
    char pwdbuf[8192];

    std::string login;

    pwdp = nullptr;
    if (options.login != "") {
        getpwnam_r(options.login.c_str(), &pwd, pwdbuf, sizeof(pwdbuf), &pwdp);
        if (pwdp == nullptr)
            PANIC("failed to resolve provided username");
        login = options.login;
    } else {
        u = getuid();
        eu = geteuid();
            getpwuid_r(u, &pwd, pwdbuf, sizeof(pwdbuf), &pwdp);
        if (pwdp == nullptr)
            PANIC("failed to retrieve username with getpwuid");
        login = pwd.pw_name;
        pwdp = nullptr;
        getpwuid_r(eu, &pwd, pwdbuf, sizeof(pwdbuf), &pwdp);
        if (pwdp == nullptr)
            PANIC("failed to retrieve effective username with getpwuid");
        login = login + " (effective " + pwd.pw_name + ")";
    }
    report.login = a2w(login.c_str());

    // TODO - collect and append the group list to the report.
}

int runner::change_credentials() {
    struct passwd *pwdp, pwd;
    char pwdbuf[8192];
    pwdp = nullptr;

    // no panics here since we are forked child
    getpwnam_r(options.login.c_str(), &pwd, pwdbuf, sizeof(pwdbuf), &pwdp);
    if (pwdp == nullptr) {
        //PANIC("failed to resolve provided username");
        return -1;
    }

    if (setregid(pwd.pw_gid, pwd.pw_gid) < 0) {
        //PANIC("failed to setregid");
        return -1;
    }
    if (setgroups(1, &pwd.pw_gid) < 0) {
        //PANIC("failed to setgroups");
        return -1;
    }
    if (setreuid(pwd.pw_uid, pwd.pw_uid) < 0) {
        //PANIC("failed to seteuid()");
        return -1;
    }
    // TODO - call get* to validate credentials (may be useless)
    return 0;
}

void runner::create_process() {
    char **argv, **envp;
    char *cmd_toexec, *cwd = nullptr;

    struct stat statbuf;

    if (options.debug)
        PANIC("debug is not supported");

    // XXX test w/ Cygwin
    if ((options.login != "") && geteuid())
        PANIC("setuid/euid requires root privileges");

    // never run a child with superuser rights.
    // this check shall be moved to the upper level (spawner_xxx.cpp)
    if ((options.login == "") && ((geteuid() == 0)))
        PANIC("use -u option");
    //    options.login = UNPRIVILEGED_USER;

    // fill login string system info
    report_login();

    std::string run_program = program;
    report.working_directory = options.working_directory;
    const char *wd = (options.working_directory != "")?options.working_directory.c_str():nullptr;
    cmd_toexec = realpath(run_program.c_str(), NULL);
    if (cmd_toexec == nullptr)
        PANIC("failed to realpath() for child program\n");

    stat(cmd_toexec, &statbuf);
    if (!S_ISREG(statbuf.st_mode))
        PANIC("please try not to exec a non regular file\n");

    // XXX move to secure runner and/or to forked child
    if (wd != nullptr) {
        cwd = getcwd(NULL, 0);
        if (cwd == nullptr)
            PANIC("failed to getcwd()");
        if (chdir(wd) == -1)
            PANIC("failed to chdir() to specified working directory");
    }
    argv = create_argv_for_process();
    envp = create_envp_for_process();

    std::string sem_name = "/spawner-sync-ch-" + std::to_string(proc_pid);
    child_sync = sem_open(sem_name.c_str(), O_CREAT | O_EXCL, O_RDWR, 0);

    auto stdinput = streams[std_stream_input]->get_pipe();
    auto stdoutput = streams[std_stream_output]->get_pipe();
    auto stderror = streams[std_stream_error]->get_pipe();

    proc_pid = fork();
    if (proc_pid == 0) { //child
        //redirect stdin
        close(STDIN_FILENO);
        if (dup2(stdinput->get_input_handle(), STDIN_FILENO) == -1) {
            PANIC(strerror(errno));
        }
        //redirect stdout
        close(STDOUT_FILENO);
        if (dup2(stdoutput->get_output_handle(), STDOUT_FILENO) == -1) {
            PANIC(strerror(errno));
        }
        //redirect stderr
        close(STDERR_FILENO);
        if (dup2(stderror->get_output_handle(), STDERR_FILENO) == -1) {
            PANIC(strerror(errno));
        }
        //close all descriptors
#ifdef __linux__
        procfd_class::close_all_pipes_without_std();
#endif

        init_process(cmd_toexec, argv, envp);
    } else if (proc_pid > 0) { // parent
        process_status = process_suspended;
        running = true;

        stdinput->close(read_mode);
        stdoutput->close(write_mode);
        stderror->close(write_mode);
    } else { // error fork()
        process_status = process_not_started;
        PANIC("failed to fork()\n");
    }

    if (wd != nullptr)
        chdir(cwd);

    // move to runner_free routine
    release_argv_for_process(argv);
    free(cmd_toexec);

    if (cwd != nullptr)
        free(cwd);

    requisites();
    sem_post(child_sync); //unlock child
    sem_close(child_sync);
    sem_unlink(sem_name.c_str());
}

void runner::runner_free() {

}

int runner::get_exit_code()
{
    if (process_status == process_spawner_crash
        || process_status == process_not_started
        || process_status == process_finished_abnormally
        || process_status == process_finished_terminated)
        return 0;

    return exit_code;
}

report_class runner::get_report() {
    report.application_name = get_program();

    report.process_status = get_process_status();
    report.signum = get_signal();
    report.exit_code = get_exit_code();

    if (ru_success) {
        report.kernel_time = (10000000 * ru.ru_stime.tv_sec) / 10 + ru.ru_stime.tv_usec;
        report.processor_time = (10000000 * ru.ru_utime.tv_sec) / 10 + ru.ru_utime.tv_usec;
        report.load_ratio = report.user_time ? (double)report.processor_time / report.user_time : 1.0;
        // "WallClock" (named "user_time") is filled in waitpid thread
#if !defined(__linux__)
        report.peak_memory_used = ru.ru_maxrss * 1024;
#endif
    }

    return report;
}

options_class runner::get_options() const {
    return options;
}
