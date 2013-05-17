#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <string>
#include <list>
#include <inc/session.h>

// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog

class options_class
{
    std::list<std::string> arguments;
public:
    session_class session;
    options_class(const session_class &session_param): 
        hide_gui(false), silent_errors(false), debug(false), secure_token(false), use_cmd(false), session(session_param), delegated(false){}
    void add_argument(std::string argument);
    void push_argument_front(std::string argument);
    std::string get_arguments() const;
    std::string string_arguments;
    std::string working_directory;
    std::string login;
    std::string password;
    std::string session_id;// only for delegate process
    bool hide_gui;
    bool debug;
    bool delegated;
	bool secure_token;
    bool silent_errors;
    bool use_cmd;// uses environment paths to find application
};





#endif//_SPAWNER_OPTIONS_H_

