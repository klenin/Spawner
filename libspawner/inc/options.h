#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <inc/session.h>

#include <string>
#include <list>
#include <vector>
// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog
// #to do: make some options as map of variant

class options_class
{
    std::list<std::string> arguments;
public:
    session_class session;
    options_class(const session_class &session_param): 
        hide_gui(false), silent_errors(false), debug(false), secure_token(false), use_cmd(false), session(session_param), 
        delegated(false), hide_report(false) {}
    void add_argument(std::string argument);
    void push_argument_front(std::string argument);
    void add_stdinput(const std::string &name);
    void add_stdoutput(const std::string &name);
    void add_stderror(const std::string &name);
    std::string get_arguments() const;
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
    bool debug;
    bool delegated;
	bool secure_token;
    bool silent_errors;
    bool use_cmd;// uses environment paths to find application
};





#endif//_SPAWNER_OPTIONS_H_

