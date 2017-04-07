#ifndef _BASE_RUNNER_H_
#define _BASE_RUNNER_H_

#include <map>
#include <string>

#include "inc/options.h"
#include "inc/status.h"
#include "inc/multibyte.h"
#include "inc/multipipe.h"
#include "inc/report.h"
#include "platform_report.h"

class base_runner {
protected:
    typedef std::list<std::pair<std::string, std::string>> env_vars_list_t;
    std::map<std_stream_type, multipipe_ptr> streams;
    bool running = false;
    bool running_async = false;
    report_class report;
    options_class options;
    process_status_t process_status = process_not_started;
    terminate_reason_t terminate_reason = terminate_reason_not_terminated;
    unsigned long long int creation_time = 0;
    std::string program;
public:
    multipipe_ptr get_pipe(const std_stream_type &stream_type);
    virtual restrictions_class get_restrictions() const {return restrictions_class(); }
    base_runner(const std::string &program, const options_class &options);
};

#endif // _BASE_RUNNER_H_
