#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <list>
#include <vector>

#include <inc/session.h>

// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog
// #to do: make some options as map of variant

struct options_class
{
    static const unsigned SHARED_MEMORY_BUF_SIZE = 4069;

    session_class session;
    options_class(const options_class &options);
    options_class(const session_class &session_param)
        : hide_gui(true)
        , silent_errors(true)
        , debug(false)
        , monitorInterval(1000) // 0.001s
        , secure_token(false)
        , use_cmd(false)
        , session(session_param)
        , delegated(false)
        , hide_report(false)
        , hide_output(false)
        , json(false)
        , controller(false)
        , environmentMode("inherit") {}
    void add_argument(std::string argument);
    void add_arguments(const std::vector<std::string> &arguments_a);
    void push_argument_front(std::string argument);
    void add_stdinput(const std::string &name);
    void add_stdoutput(const std::string &name);
    void add_stderror(const std::string &name);
    void add_environment_variable(const std::string &envStr);
    void clear_stdinput();
    void clear_stdoutput();
    void clear_stderror();
    std::string get_arguments() const;
    std::string get_argument(const size_t &index) const;
    size_t get_arguments_count() const;
    std::string format_arguments() const;
    std::string string_arguments;
    std::string working_directory;
    std::string login;
    std::string password;
    std::vector<std::string> stdinput;
    std::vector<std::string> stdoutput;
    std::vector<std::string> stderror;
    std::list< std::pair< std::string, std::string > > environmentVars;
    std::list<std::string> arguments;
    std::string report_file; //bad, need many values but this causes many outputs if environment variable and command line argument both present
    std::string shared_memory;
    std::string mutex;
    bool hide_gui;
    bool hide_report;
    bool hide_output;
    bool debug;
    bool json;
    bool secure_token;
    bool silent_errors;
    bool use_cmd;
    bool delegated;
    bool controller;
    unsigned long monitorInterval;
    std::string environmentMode;
};

#endif //_SPAWNER_OPTIONS_H_

