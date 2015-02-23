#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <inc/session.h>

#include <list>
#include <vector>
// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog
// #to do: make some options as map of variant

class options_class
{
protected:
    std::list<std::string> arguments;
public:
    session_class session;
    options_class(const options_class &options);
    options_class(const session_class &session_param): 
        hide_gui(true), silent_errors(true), debug(false), secure_token(false), use_cmd(false), session(session_param), 
        delegated(false), hide_report(false), hide_output(false), json(false) {}
    void add_argument(std::string argument);
    void add_arguments(const std::vector<std::string> &arguments_a);
    void push_argument_front(std::string argument);
    void add_stdinput(const std::string &name);
    void add_stdoutput(const std::string &name);
    void add_stderror(const std::string &name);
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
    std::string session_id;// only for delegate process
    std::vector<std::string> stdinput;
    std::vector<std::string> stdoutput;
    std::vector<std::string> stderror;
    std::string report_file;//bad, need many values but this causes many outputs if environment variable and command line argument both present
    bool hide_gui;
    bool hide_report;
    bool hide_output;
    bool debug;
    bool json;
    bool delegated;
    bool secure_token;
    bool silent_errors;
    bool use_cmd;// uses environment paths to find application
};





#endif//_SPAWNER_OPTIONS_H_

