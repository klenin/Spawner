#include "platform.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "options.h"

int get_spawner_pid() {
	return (int)getpid();
}

void push_shm_report(const char *shm_name, const std::string &report)
{
	int shm_fd = shm_open(shm_name, O_RDWR, 0666);
	if (shm_fd == -1) {
		/*
		switch(errno) {
		case EACCES: std::cout << "eacces"; break;
		case EEXIST: std::cout << "eexist"; break;
		case EINVAL: std::cout << "einval"; break;
		case EMFILE: std::cout << "emfile"; break;
		case ENFILE: std::cout << "enfile"; break;
		case ENOENT: std::cout << "enoent"; break;
		default: break;
		}
		*/
		PANIC("Failed to shm_open() while pusing the report");
	}
	void *report_addr = mmap(0, options_class::SHARED_MEMORY_BUF_SIZE,
	    PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (report_addr == MAP_FAILED) {
		/*
		switch(errno) {
		case EACCES:    std::cout << "eacces"; break;
		case EAGAIN:    std::cout << "eagain"; break;
		case EBADF:     std::cout << "ebadf"; break;
		case EINVAL:    std::cout << "einval"; break;
		case ENFILE:    std::cout << "enfile"; break;
		case ENODEV:    std::cout << "enodev"; break;
		case ENOMEM:    std::cout << "enomem"; break;
		case EPERM:     std::cout << "eperm"; break;
		case ETXTBSY:   std::cout << "etxtbsy"; break;
		case EOVERFLOW: std::cout << "eoverflow"; break;
default: break;
		}
		*/
		PANIC("Failed to mmap() while pushing the report");
	}
	memcpy(report_addr, report.c_str(), sizeof(char) * report.length());
	munmap(report_addr, (size_t)options_class::SHARED_MEMORY_BUF_SIZE);
}

void pull_shm_report(const char *shm_name, std::string &report) {
	int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
	if (shm_fd == -1) {
		/*
		switch(errno) {
		case EACCES: std::cout << "eacces"; break;
		case EEXIST: std::cout << "eexist"; break;
		case EINVAL: std::cout << "einval"; break;
		case EMFILE: std::cout << "emfile"; break;
		case ENFILE: std::cout << "enfile"; break;
		case ENOENT: std::cout << "enoent"; break;
                default: break;
                }
		*/
		PANIC("Panic: failed to shm_open() for reading");
	}
	void *report_addr = mmap(0, options_class::SHARED_MEMORY_BUF_SIZE,
	    PROT_READ, MAP_SHARED, shm_fd, 0);
	if (report_addr == MAP_FAILED) {
                /*
		switch(errno) {
		case EACCES:    std::cout << "eacces"; break;
		case EAGAIN:    std::cout << "eagain"; break;
		case EBADF:     std::cout << "ebadf"; break;
		case EINVAL:    std::cout << "einval"; break;
		case ENFILE:    std::cout << "enfile"; break;
		case ENODEV:    std::cout << "enodev"; break;
		case ENOMEM:    std::cout << "enomem"; break;
		case EPERM:     std::cout << "eperm"; break;
		case ETXTBSY:   std::cout << "etxtbsy"; break;
		case EOVERFLOW: std::cout << "eoverflow"; break;
default: break;
		}
		*/
		PANIC("Panic: failed to mmap() while pulling report");
	} else
		report = (const char *)report_addr;
	munmap(report_addr, (size_t)options_class::SHARED_MEMORY_BUF_SIZE);
	shm_unlink(shm_name);
}

size_t get_env_var(const char *name, char *buff, size_t size) {
	char *s = nullptr;
	int result = 0;

	s = getenv(name);
	if (s != nullptr) {
		result = strnlen(s, size);
		if (result) {
			strncpy(buff, s, result);
			return result;
		}
	}

	return result;
}

std::string ExtractExitStatus(const report_class &report) {
	std::ostringstream s;

	if (report.signum)
		s << get_signal_info(report.signum, "%n (%t)");
	else
		s << report.exit_code;

	return s.str();
}
