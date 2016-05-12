#ifndef _RLIMIT_H_
#define _RLIMIT_H_
#include <sys/time.h>
#include <sys/resource.h>

#define RL_FAIL_SET 1
#define RL_FAIL_GET 2
#define RL_FAIL_VALIDATE 3


int impose_rlimit(int, long);
#endif // _RLIMIT_H_
