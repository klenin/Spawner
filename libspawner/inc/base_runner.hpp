#ifndef _SP_BASE_RUNNER_HPP_
#define _SP_BASE_RUNNER_HPP_

#include <map>
#include <string>

#include "inc/options.hpp"
#include "inc/status.hpp"
#include "inc/multibyte.hpp"
#include "inc/pipes.hpp"

#include "platform_report.hpp"

class base_runner {
protected:
    typedef std::list<std::pair<std::string, std::string>> env_vars_list_t;
    std::map<pipes_t, std::shared_ptr<pipe_c>> pipes;
    bool running = false;
    bool running_async = false;
    report_class report;
    options_class options;
    process_status_t process_status = process_not_started;
    unsigned long long int creation_time;
    std::string program;
public:
    void set_pipe(const pipes_t &pipe_type, std::shared_ptr<pipe_c> pipe_object);
    std::shared_ptr<pipe_c> get_pipe(const pipes_t &pipe_type);
    std::shared_ptr<input_pipe_c> get_input_pipe();
    std::shared_ptr<output_pipe_c> get_output_pipe();
    std::vector<std::shared_ptr<duplex_buffer_c>> duplex_buffers;
    virtual restrictions_class get_restrictions() const {return restrictions_class(); }
    base_runner(const std::string &program, const options_class &options);
};

#endif // _SP_BASE_RUNNER_HPP_
