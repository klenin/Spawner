#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>
#include <list>

class COptions
{
public:
    COptions():show_gui(true), silent_errors(true){}
    void add_argument(std::string argument);
    std::string get_arguments();
    std::list<std::string> arguments;
    std::string working_directory;
    bool show_gui;
    bool silent_errors;
};





#endif//_OPTIONS_H_

