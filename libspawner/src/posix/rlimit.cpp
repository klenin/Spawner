#include "rlimit.h"

int impose_rlimit(int resource, long limit) {
	struct rlimit rl;

	rl.rlim_max = limit;
	rl.rlim_cur = limit;

	if (setrlimit(resource, &rl))
		return 1;
	if (getrlimit(resource, &rl))
		return 2; 
	if ((rl.rlim_cur != limit) || (rl.rlim_max != limit))
		return 3;

	return 0; 
}
