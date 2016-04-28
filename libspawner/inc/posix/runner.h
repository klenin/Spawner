#ifndef _RUNNER_H_
#define _RUNNER_H_

#include <sys/resource.h>

#include "inc/base_runner.h"

#if defined(__linux__)
#include "linux_affinity.h"
#include "linux_seccomp.h"
#endif


class runner: public base_runner {
private:
	pthread_t waitpid_thread;
	pid_t proc_pid;

        struct rusage ru;  // precise resource usage storage
        bool ru_success = false;

	// sync pipe descriptors 
        int child_sync[2];
	int child_syncbuf = 42;

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
	mutable pthread_mutex_t envp_lock;

	pthread_mutex_t waitpid_lock;
	pthread_cond_t waitpid_cond;
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
	void suspend() { };
	void resume() { };
	bool start_suspended = true;
	virtual process_status_t get_process_status();
	signal_t get_signal();
	virtual unsigned long long get_time_since_create();
	static unsigned long long get_current_time();
	int get_exit_code();
};
#endif
