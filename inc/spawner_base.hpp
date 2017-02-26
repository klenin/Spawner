#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "sp.hpp"
#include "arguments.hpp"
#include "inc/compatibility.hpp"

class spawner_base_c {
protected:
    std::vector<runner*> runners;
    std::map<std::string, runner*> runner_alias;
    std::shared_ptr<output_buffer_c> create_output_buffer(const std::string &name,
        const pipes_t &pipe_type);
    std::shared_ptr<input_buffer_c> create_input_buffer(const std::string &name);

public:
    spawner_base_c();
    virtual ~spawner_base_c();
    virtual void begin_report();
    virtual void run(int argc, char *argv[]);
    virtual std::string help();
    virtual void init_arguments();
    virtual bool init();
    virtual void print_report();
    virtual void run();
};
