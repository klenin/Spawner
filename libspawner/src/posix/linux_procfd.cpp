#include "linux_procfd.h"

#include <cstdio>
#include <unistd.h>
#include <dirent.h>

void procfd_class::close_all_pipes_without_std() {
    char dir[100];
    std::sprintf(dir, "/proc/%d/fd/", getpid());

    auto dp = opendir(dir);
    if (dp != NULL) {
        while (auto ep = readdir (dp)) {
            int piped = atoi(ep->d_name);
            if (piped > STDERR_FILENO) { // save stdin(0) stdout(1) stderr(2)
                close(piped); //close other
            }
        }
        closedir(dp);
    }
}
