#ifndef _SP_LINUX_AFFINITY_HPP_
#define _SP_LINUX_AFFINITY_HPP_

#include <sched.h>

class linux_affinity_class {
private:
    cpu_set_t cpumask;
public:
    bool set(pid_t p);
};

#endif // _SP_LINUX_AFFINITY_HPP_
