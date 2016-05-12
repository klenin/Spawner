#ifndef _SPAWNER_REPORT_H_
#define _SPAWNER_REPORT_H_

#include <string>

#include "status.h"
#include "options.h"
#include "restrictions.h"

class base_report_class
{
public:
    base_report_class():
        process_status(process_not_started),
        terminate_reason(terminate_reason_not_terminated),
        peak_memory_used(0),
        write_transfer_count(0),
        exit_code(0),
        total_time(0),
        load_ratio(0.0),
        processor_time(0),
        user_time(0),
        kernel_time(0) {}
    process_status_t process_status;
    
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

std::string get_status_text(process_status_t process_status);
std::string get_terminate_reason(terminate_reason_t terminate_reason);

struct map_cell
{
    unsigned long key;
    const char *name;
    const char *text;
};

const unsigned int process_status_descriptions_count = 7;
const unsigned int terminate_reason_descriptions_count = 7;

struct process_status_description
{
    process_status_t process_status;
    const char *name;
};

const process_status_description process_status_descriptions[] = {
    {process_still_active,        "PROCESS_STILL_ACTIVE"},
    {process_suspended,           "PROCESS_SUSPENDED"},
    {process_finished_normal,     "PROCESS_FINISHED_NORMAL"},
    {process_finished_abnormally, "PROCESS_FINISHED_ABNORMALLY"},
    {process_finished_terminated, "PROCESS_FINISHED_TERMINATED"},
    {process_not_started,         "PROCESS_NOT_STARTED"},
    {process_spawner_crash,       "PROCESS_SPAWNER_CRASH"},
};

struct terminate_reason_description
{
    terminate_reason_t terminate_reason;
    const char *name;
};

const terminate_reason_description terminate_reason_descriptions[] = {
    {terminate_reason_not_terminated,           "ExitProcess"},//TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_none,                     "<none>"},//TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_abnormal_exit_process,    "AbnormalExitProcess"},//TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_time_limit,               "TimeLimitExceeded"},//"TERMINATE_REASON_TIME_LIMIT"},
    {terminate_reason_write_limit,              "WriteLimitExceeded"},//"TERMINATE_REASON_WRITE_LIMIT"},
    {terminate_reason_memory_limit,             "MemoryLimitExceeded"},//"TERMINATE_REASON_MEMORY_LIMIT"},
    {terminate_reason_user_time_limit,          "TimeLimitExceeded"},//"TERMINATE_REASON_USER_TIME_LIMIT"},
    {terminate_reason_load_ratio_limit,         "IdleTimeLimitExceeded"},//"TERMINATE_REASON_LOAD_RATIO_LIMIT"},
    {terminate_reason_debug_event,              "DebugEvent"},//"TERMINATE_REASON_DEBUG_EVENT"},
    {terminate_reason_created_process,          "ProcessesCountLimitExceeded"},
    {terminate_reason_not_terminated,           nullptr}
};

#endif//_SPAWNER_REPORT_H_
