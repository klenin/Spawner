#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <unistd.h>
#include <stdio.h>

#include <sys/types.h> // open, waitpid
#include <sys/wait.h>

#include <unistd.h> // getpagesize(), access()

#include "inc/procfs.h"

#define TRUE 1
#define FALSE 0
#define INFINITE 0

procfs_class::procfs_class()
{
	proc_discovered = FALSE;
	proc_disappeared = FALSE;
	page_size = getpagesize();
}

bool procfs_class::ready() {
	return (proc_discovered);
}

void procfs_class::probe_pid(pid_t p)
{
	using namespace std;
	
	stat_path = "/proc/" + to_string(p) + "/stat";
	io_path = "/proc/" + to_string(p) + "/io";
	statm_path = "/proc/" + to_string(p) + "/statm";
	
	if ((access(stat_path.c_str(), R_OK) != -1) &&
		(access(io_path.c_str(), R_OK) != -1) &&
		(access(statm_path.c_str(), R_OK) != -1))
		proc_discovered = TRUE;
	else
		std::cout << "procfs stat files are not available for pid " << p;
}

bool procfs_class::fill_all() {
	if (fill_statm() && fill_io() && fill_stat())
		return TRUE;
	proc_disappeared = TRUE;
	return FALSE;
}

bool procfs_class::fill_io()
{
	std::string str;
	size_t value;
	io_stream.open(io_path);
	if (io_stream.good()) {
		while (io_stream >> str >> value)
			if (str == "read_bytes:") {
				//std::cout << "results: " << value << " str: " << str << "\n";
				read_bytes = value;
				continue;
			} else if (str == "write_bytes:") {
				//std::cout << "results: " << value << " str: " << str << "\n";
				write_bytes = value;
				io_stream.close();
				return 1;
			}
	} else 
		return 0;
}
bool procfs_class::fill_stat()
{
	// it is better to use tokenizer here.
	std::string value;
	static const int utime_pos = 13, stime_pos = 14, vsize_pos = 22, rss_pos = 23;
	int index = 0;

	stat_stream.open(stat_path);
	if(stat_stream.good()) {
		while ((stat_stream >> value) && (index <= rss_pos)) {
			switch (index++) {
			case utime_pos: stat_utime = std::stoi(value); break;
			case stime_pos: stat_stime = std::stoi(value); break;
			case vsize_pos: stat_vsize = std::stoi(value); break;
			case rss_pos:   stat_rss = std::stoi(value) * page_size; break;
			default: break;
			}
		}
		std::cout << "utime: " << stat_utime << " stat_stime: " << stat_stime << " stat_vsize " << stat_vsize << " stat_rss " << stat_rss << "\n" ;

		stat_stream.close();
		return 1;
	} else {
		proc_discovered = FALSE;
		return 0;
	}
}
bool procfs_class::fill_statm()
{
	size_t vsize, resident;
	
	statm_stream.open(statm_path);
	if (statm_stream.good()) {
		if (statm_stream >> vsize >> resident) {
			mem_vsize = vsize * page_size;
			mem_resident = resident * page_size;
			std::cout << "vsize " << mem_vsize << " resident " << mem_resident << "\n";
			statm_stream.close();
			return 1;
		}
	}
	proc_discovered = FALSE;
	return 0;
}

void procfs_class::close_handlers() {
/*	std::cout << "close_handlers";
	if (proc_discovered) {
		io_stream.close();
		stat_stream.close();
		statm_stream.close();
	}
*/
}
