#ifndef _SPAWNER_REPORT_H_
#define _SPAWNER_REPORT_H_

#include <string>
#include "status.h"
#include "options.h"
#include "restrictions.h"

#if !defined(_WIN32) && !defined(_WIN64)
const char *get_signal_name(signal_t signal);
const char *get_signal_text(signal_t signal);
std::string get_signal_info(signal_t signal, std::string format);
#else
const char *get_exception_name(exception_t exception);
const char *get_exception_text(exception_t exception);
std::string get_exception_info(exception_t exception, std::string format);
#endif

std::string get_status_text(process_status_t process_status);
std::string get_terminate_reason(terminate_reason_t terminate_reason);

class report_class// <-- struct?
{
public:
#if defined(_WIN32) || defined(_WIN64)
    report_class():process_status(process_not_started), exception(exception_exception_no), terminate_reason(terminate_reason_not_terminated), peak_memory_used(0),
#else
    report_class():process_status(process_not_started), signum(signal_signal_no), terminate_reason(terminate_reason_not_terminated), peak_memory_used(0),
#endif
    write_transfer_count(0), exit_code(0), total_time(0), load_ratio(0.0), processor_time(0), user_time(0), kernel_time(0){}
    process_status_t process_status;
    
#if defined(_WIN32) || defined(_WIN64)
    exception_t exception;
#else
    signal_t signum;
#endif
    terminate_reason_t terminate_reason;
    //may be move this to different structure
    unsigned long peak_memory_used;
    unsigned long long write_transfer_count;
//  size_t read_transfer_count;
    unsigned int exit_code;
    size_t total_time;
    double load_ratio;
    unsigned long long processor_time;
    unsigned long long user_time;
    // additional info subset
    unsigned long long kernel_time;
    // options subset
    std::string application_name;
    std::wstring login;
    std::string working_directory;
};

#endif//_SPAWNER_REPORT_H_
