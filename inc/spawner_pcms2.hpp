#pragma once

#include <string>

#include "spawner_old.hpp"

class spawner_pcms2_c: public spawner_old_c {
public:
    spawner_pcms2_c(settings_parser_c &parser);
    virtual void begin_report();
    virtual bool init();
    virtual void print_report();
    virtual std::string help();
    virtual void init_arguments();
    virtual void init_std_streams();
};
