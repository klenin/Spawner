#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

class procfs_class {
private:
	bool proc_discovered, proc_disappeared;
	int page_size, mem_vsize, mem_resident;
	size_t read_bytes, write_bytes; // io file: i/o from block-backed storages 
	size_t stat_utime, stat_stime;
	size_t stat_vsize, stat_rss;
	
	std::string io_path, stat_path, statm_path;
	std::ifstream stat_stream, io_stream, statm_stream;

public:
	void probe_pid(pid_t);
	procfs_class();
	bool fill_statm();
	bool fill_stat();
	bool fill_io();
	bool fill_all();
	bool ready();
	void close_handlers();
};
