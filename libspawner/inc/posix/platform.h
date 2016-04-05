#ifndef _POSIX_PLATFORM_H_
#define _POSIX_PLATFORM_H_
#include <string>
#include "inc/report.h"

// oostream from stdlibc++/gcc4.9 can not work with wide strings
#define platform_login_str(x) w2a(x)

int get_spawner_pid();

void push_shm_report(const char *, const std::string &);
void pull_shm_report(const char *, std::string &);

size_t get_env_var(const char *, char *, size_t);
std::string ExtractExitStatus(const report_class &);

#endif // _POSIX_PLATFORM_H_
