#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <string>
#include <list>

// show_gui - if disabled< windows created by process are not shown
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog

class COptions
{
    std::list<std::string> arguments;
public:
    COptions():show_gui(true), silent_errors(true){}
    void add_argument(std::string argument);
    std::string get_arguments();
    std::string string_arguments;
    std::string working_directory;
    bool show_gui;
    bool silent_errors;
};





#endif//_SPAWNER_OPTIONS_H_

