#ifndef _POSIX_PLATFORM_H_
#define _POSIX_PLATFORM_H_
#include <string>
#include <stdlib.h>

#ifdef __MACH__
#include <sys/time.h>
#endif

#include "platform_report.h"

#define FOREGROUND_BLUE 0
#define FOREGROUND_GREEN 0
#define FOREGROUND_RED 0
#define FOREGROUND_INTENSITY 0

typedef int pipe_handle;

typedef void thread_return_t;
typedef void* thread_param_t;
typedef pthread_t thread_t;

int get_spawner_pid();

void push_shm_report(const char *, const std::string &);
void pull_shm_report(const char *, std::string &);

size_t get_env_var(const char *, char *, size_t);
std::string ExtractExitStatus(const report_class &);

void platform_exit_failure();

void platform_init();

#ifdef __MACH__
#define CLOCK_REALTIME 0
unsigned long long int clock_gettime(int, struct timespec *);
#endif

#endif // _POSIX_PLATFORM_H_
