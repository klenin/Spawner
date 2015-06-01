#pragma once

#include <string>
#include <vector>

#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

#include "arguments.h"
#include "sp.h"
#include "spawner_base.h"

class spawner_new_c: public spawner_base_c {
protected:
    restrictions_class restrictions;
    options_class options;
    restrictions_class base_restrictions;
    options_class base_options;
    bool runas;
    bool base_initialized;
    settings_parser_c &parser;
    std::vector<runner*> runners;
    size_t order;

public:
    spawner_new_c(settings_parser_c &parser);
    virtual ~spawner_new_c();

    void json_report(runner *runner_instance, rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > &writer);
    virtual bool init();
    bool init_runner();
    virtual void run();
    virtual void print_report();
    virtual std::string help();
    virtual void on_separator(const std::string &_);
    virtual void init_arguments();
};
