#ifndef _POSIX_PLATFORM_H_
#define _POSIX_PLATFORM_H_
#include <string>

int get_spawner_pid();

void push_shm_report(const char *, const std::string &);
void pull_shm_report(const char *, std::string &);

size_t get_env_var(const char *, char *, size_t);
#endif // _POSIX_PLATFORM_H_
