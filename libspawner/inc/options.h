#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <list>
#include <vector>

#include <inc/session.h>

// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog

// TODO: make some options as map of variant
// TODO: rewrite as a class

struct options_class
{
    static const unsigned SHARED_MEMORY_BUF_SIZE = 4069;

    session_class session;
    std::string report_file;  /* bad, need many values
                                 but this causes many outputs if environment
                                 variable and command line argument both present */
    std::string shared_memory;
    
    std::string string_arguments;
    std::string working_directory;
    
    std::string login;
    std::string password;
    
    std::vector<std::string> stdinput;
    std::vector<std::string> stdoutput;
    std::vector<std::string> stderror;
    std::list<std::string> arguments;

    std::list< std::pair< std::string, std::string > > environmentVars;
    std::string environmentMode = "inherit";
       
    bool hide_gui = true;
    bool hide_report = false;
    bool hide_output = false;
    bool debug = false;
    bool json = false;
    bool secure_token = false;
    bool silent_errors = true;
    bool use_cmd = false;
    bool delegated = false;
    bool controller = false;
    unsigned long monitorInterval = 1000; // 0.001s
 
    options_class(const session_class &session_param) : session(session_param) {}

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
    std::string get_argument(size_t index) const;
    size_t get_arguments_count() const;
    std::string format_arguments() const;
};

#endif //_SPAWNER_OPTIONS_H_

