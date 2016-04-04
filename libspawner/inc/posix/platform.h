#ifndef _POSIX_PLATFORM_H_
#define _POSIX_PLATFORM_H_
#include <string>

int get_spawner_pid();

void push_shm_report(const char *, const std::string &);
void pull_shm_report(const char *, std::string &);

#endif // _POSIX_PLATFORM_H_
