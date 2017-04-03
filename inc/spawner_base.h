#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "sp.h"
#include "arguments.h"
#include "inc/compatibility.h"

class spawner_base_c {
    pipe_broadcaster_ptr spawner_stdin;
    pipe_broadcaster_ptr spawner_stdout;
    pipe_broadcaster_ptr spawner_stderr;

protected:
    std::vector<runner*> runners;
    std::map<std::string, runner*> runner_alias;
    std::map<std::string, pipe_broadcaster_ptr> file_pipes;

    pipe_broadcaster_ptr get_or_create_file_pipe(const std::string& path, pipe_mode mode);
    pipe_broadcaster_ptr get_std(std_stream_type type);

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
