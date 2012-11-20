#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <string>
#include <list>

// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog

class options_class
{
    std::list<std::string> arguments;
public:
    options_class(): hide_gui(false), silent_errors(false), debug(false), secure_token(false), use_cmd(false){}
    void add_argument(std::string argument);
    std::string get_arguments() const;
    std::string string_arguments;
    std::string working_directory;
    std::string login;
    std::string password;
    bool hide_gui;
    bool debug;
	bool secure_token;
    bool silent_errors;
    bool use_cmd;// uses environment paths to find application
};





#endif//_SPAWNER_OPTIONS_H_

