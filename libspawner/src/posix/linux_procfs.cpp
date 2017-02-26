#include "linux_procfs.hpp"

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

bool procfs_class::probe_pid(pid_t p)
{
    using namespace std;

    stat_path = "/proc/" + to_string(p) + "/stat";
    io_path = "/proc/" + to_string(p) + "/io";

    if ((access(stat_path.c_str(), R_OK) != -1) &&
        (access(io_path.c_str(), R_OK) != -1))
        discovered = true;
    else
        discovered = false;

    return discovered;
}

bool procfs_class::fill_all()
{
    if (fill_io() && fill_stat())
        return true;

    disappeared = true;

    return false;
}

bool procfs_class::fill_io() {
    char buffer[4096], *needle;
    int fd, len;
    size_t io_read, io_write;

    fd = open(io_path.c_str(), O_RDONLY);
    if (fd < 0) {
        disappeared = true;
        return false;
    }
    len = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (len <= 0) {
        disappeared = true;
        return false;
    }
    buffer[len] = '\0';

/*
    needle = strstr(buffer, IO_RD_STR);
    if (needle == nullptr) {
        disappeared = true;
        return false;
    }
    errno = 0;
    io_read = strtoull(needle + strlen(IO_RD_STR), nullptr, 10);
    if (errno) {
        disappeared = true;
        return false;
    }
    read_bytes = io_read;
*/

    needle = strstr(buffer, IO_WR_STR);
    if (needle == nullptr) {
        disappeared = true;
        return false;
    }
    errno = 0;
    io_write = strtoull(needle + strlen(IO_WR_STR), nullptr, 10);
    if (errno) {
        disappeared = true;
        return false;
    }
    write_bytes = io_write;

    //printf("%lu, %lu\n", io_read, io_write);
    return true;
}

bool procfs_class::fill_stat() {
    char buffer[4096], *token=buffer;
    size_t utime, stime, vsize, rss;
    int fd, len, index = 0;

    fd = open(stat_path.c_str(), O_RDONLY);
    if (fd < 0) {
        disappeared = true;
        return false;
    }
    len = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (len <= 0) {
        disappeared = true;
        return false;
    }
    buffer[len] = '\0';
    do {
        errno = 0;
        switch (index++) {
        case STAT_UTIME_POS: utime=strtoull(token, nullptr, 10); break;
        case STAT_STIME_POS: stime=strtoull(token, nullptr, 10); break;
        case STAT_VSIZE_POS: vsize=strtoull(token, nullptr, 10); break;
        case STAT_RSS_POS:   rss=strtoull(token, nullptr, 10); break;
        default: break;
        }
        if (errno) {
            disappeared = true;
            return false;
        }
    } while ((strsep(&token, " ") != nullptr) || (index <= STAT_LAST));

    stat_utime = utime;
    stat_stime = stime;
    stat_vsize = vsize;
    stat_rss   = rss;

    if (vsize > vss_max)
        vss_max = vsize;
    //printf("%lu %lu %lu %lu\n", utime, stime, vsize, rss);

    return true;
}
