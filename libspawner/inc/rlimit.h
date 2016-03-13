#ifndef _RLIMIT_H_
#define _RLIMIT_H_
#include <sys/time.h>
#include <sys/resource.h>

int impose_rlimit(int, long);
#endif // _RLIMIT_H_
