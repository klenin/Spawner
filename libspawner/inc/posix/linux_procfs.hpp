#ifndef _SP_LINUX_PROCFS_HPP_
#define _SP_LINUX_PROCFS_HPP_

#include <string>

#define STAT_UTIME_POS 13
#define STAT_STIME_POS 14
#define STAT_VSIZE_POS 22
#define STAT_RSS_POS 23

#define STAT_LAST STAT_RSS_POS

#define IO_RD_STR "read_bytes: "
#define IO_WR_STR "write_bytes: "


struct procfs_class {
    bool discovered, disappeared;
    size_t read_bytes, write_bytes; // io file: i/o from block-backed storages 
    size_t stat_utime, stat_stime;
    size_t stat_vsize, stat_rss;
    size_t vss_max;

    std::string io_path, stat_path;

    bool probe_pid(pid_t);
    bool fill_stat();
    bool fill_io();
    bool fill_all();
};

#endif // _SP_LINUX_PROCFS_HPP_
