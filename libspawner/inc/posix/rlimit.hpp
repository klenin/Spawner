#ifndef _SP_RLIMIT_HPP_
#define _SP_RLIMIT_HPP_

#include <sys/time.h>
#include <sys/resource.h>

#define RL_FAIL_SET 1
#define RL_FAIL_GET 2
#define RL_FAIL_VALIDATE 3


int impose_rlimit(int, long);

#endif // _SP_RLIMIT_HPP_
