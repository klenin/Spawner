#include <sched.h>

class linux_affinity_class {
private:
	cpu_set_t cpumask;
public:
	bool set(pid_t p);
};
