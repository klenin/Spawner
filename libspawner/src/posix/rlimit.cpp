#include "rlimit.hpp"

int impose_rlimit(int resource, long limit) {
    struct rlimit rl;
    long hard;

    hard = limit;
    if (resource == RLIMIT_CPU)
        ++hard;

    rl.rlim_cur = limit;
    rl.rlim_max = hard;

    if (setrlimit(resource, &rl))
        return RL_FAIL_SET;
    if (getrlimit(resource, &rl))
        return RL_FAIL_GET; 
    if ((rl.rlim_cur != limit) || (rl.rlim_max != hard))
        return RL_FAIL_VALIDATE;

    return 0; 
}
