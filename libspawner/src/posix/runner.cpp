#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include "inc/error.h"

#include "runner.h"

runner::runner (const std::string &program, const options_class &options)
	: base_runner(program, options)
{
	pthread_mutex_init(&envp_lock, NULL);
	pthread_mutex_init(&waitpid_lock, NULL);
	pthread_cond_init(&waitpid_cond, NULL);
}
runner::~runner() {
	pthread_mutex_destroy(&envp_lock);
	pthread_mutex_destroy(&waitpid_lock);
	pthread_cond_destroy(&waitpid_cond);
}

process_status_t runner::get_process_status(){
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
	pthread_mutex_lock(&envp_lock);
	environ = nullptr; //dont call clearenv() since it releases memory
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

	for (auto i = options.environmentVars.cbegin(); i != options.environmentVars.cend(); ++i) {
		setenv(i->first.c_str(), i->second.c_str(), 1);
	}

	// restore original environ pointer
	result = environ;
	environ = original;
	pthread_mutex_unlock(&envp_lock);

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
	close(child_sync[1]);

	// try to change credentials, exit on fail.
	// TODO - return value over pipe
	if (options.login != "")
		if (change_credentials())
			exit(EXIT_FAILURE);
	if (read(child_sync[0], &child_syncbuf, sizeof(child_syncbuf)) == -1) {
		// no panics since we are child, just kill itself 
		//PANIC("failed get sync message from parent");
		exit(EXIT_FAILURE);
	}
	close(child_sync[0]);
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

	// report that waitpid thread has been started
	pthread_mutex_lock(&self->waitpid_lock);
	self->waitpid_ready = true;
	pthread_mutex_unlock(&self->waitpid_lock);
	pthread_cond_signal(&self->waitpid_cond);

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
	if (getrusage(RUSAGE_CHILDREN, &self->ru) != -1)
		self->ru_success = true;

	pthread_exit(NULL);
}

void runner::run_process_async() {
	running_async = true;
	create_process();
	requisites();
}

bool runner::wait_for()
{
	pthread_join(waitpid_thread, NULL);
	running = false;
}

void runner::requisites() {
#if defined(__linux__)
	affinity.set(proc_pid);
#endif
	// wait till waitpid body completely starts
        pthread_mutex_lock(&waitpid_lock);
        pthread_create(&waitpid_thread, NULL, waitpid_body, (void *)this);
        while (!waitpid_ready)
                pthread_cond_wait(&waitpid_cond, &waitpid_lock);
        pthread_mutex_unlock(&waitpid_lock);

	// child is now blocked with read() on other end of the pipe. 
	close(child_sync[0]);
	write(child_sync[1], &child_syncbuf, sizeof(child_syncbuf));
	process_status = process_still_active;
	close(child_sync[1]);
}

void runner::report_login() {
        uid_t u, eu;
	
        struct passwd *pwdp, pwd;
        char pwdbuf[8192];

	std::string login;
	
	pwdp == nullptr;
	if (options.login != "") {
		getpwnam_r(options.login.c_str(), &pwd, pwdbuf, sizeof(pwdbuf),
		     &pwdp);
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
	//	options.login = UNPRIVILEGED_USER;

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

	if (pipe(child_sync) == -1)
		PANIC("failed to create pipe() for syncing with the child\n");
	proc_pid = fork();
	if (proc_pid == -1) {
		process_status = process_not_started;
		PANIC("failed to fork()\n");
	}
	if (proc_pid) {
		process_status = process_suspended;
		running = true;
	} else
		init_process(cmd_toexec, argv, envp);
	
	if (wd != nullptr)
		chdir(cwd);

	// move to runner_free routine
	release_argv_for_process(argv);
	free(cmd_toexec);

	if (cwd != nullptr)
		free(cwd);

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

