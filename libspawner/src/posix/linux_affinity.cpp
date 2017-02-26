#include "linux_affinity.hpp"

bool linux_affinity_class::set(pid_t p)
{
    CPU_ZERO(&cpumask);
    CPU_SET(0, &cpumask); // add only a first core
    
    return ((sched_setaffinity(p, sizeof(cpumask), &cpumask) != -1)) ? 1 : 0;
}
