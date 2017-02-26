#include "linux_seccomp.hpp"

int seccomp_probe_filter() {
    int rval;

    errno = 0;
    rval = prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, NULL, 0, 0);

    // 0 is ok, 1 on failure
    return ((rval < 0) && (errno == EFAULT)) ? 0 : 1;
}

int seccomp_setup_filter() {
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

    return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
}
