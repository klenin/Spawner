#pragma once

#include <string>

#include "sp.h"
#include "spawner_base.h"

class spawner_old_c: public spawner_base_c {
protected:
    restrictions_class restrictions;
    options_class options;
    bool runas;
    settings_parser_c &parser;
    std::string report_file;
    std::string output_file;
    std::string error_file;
    std::string input_file;
    secure_runner *secure_runner_instance;

public:
    spawner_old_c(settings_parser_c &parser);
    virtual void init_std_streams();
    virtual ~spawner_old_c();
    virtual bool init();
    virtual void run();
    virtual void print_report();
    virtual std::string help();
    virtual void init_arguments();
};
