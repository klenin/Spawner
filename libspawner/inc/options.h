#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <string>
#include <list>

// show_gui - if disabled< windows created by process are not shown
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog

class options_class
{
    std::list<std::string> arguments;
public:
    options_class():hide_gui(false), silent_errors(false), debug(false){}
    void add_argument(std::string argument);
    std::string get_arguments();
    std::string string_arguments;
    std::string working_directory;
    std::string login;
    std::string password;
    bool hide_gui;
    bool debug;
    bool silent_errors;
};





#endif//_SPAWNER_OPTIONS_H_

