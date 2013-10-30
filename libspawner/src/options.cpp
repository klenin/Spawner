#include "options.h"
#include <algorithm>
void options_class::add_argument(std::string argument) {
    arguments.push_back(argument);
}

void options_class::add_arguments(const std::vector<std::string> &arguments_a) {
    for (int i = 0; i < arguments_a.size(); ++i) {
        arguments.push_back(arguments_a[i]);
    }
}

void options_class::push_argument_front(std::string argument) {
    arguments.push_front(argument);
}

std::string options_class::get_arguments() const
{
    if (arguments.size() == 0)
        return "";
    std::string result = arguments.front();
    std::list<std::string>::const_iterator it = arguments.begin();
    while (++it != arguments.end())
        result += " " + *it;
    return result;
}


std::string options_class::format_arguments() const
{
    std::string arguments_string = get_arguments();
    if (arguments_string == "") {
        return "<none>";
    }
    return arguments_string;
}

//TODO! rethink this
void options_class::add_stdinput(const std::string &name) {
    if (std::find(stdinput.begin(), stdinput.end(), name) != stdinput.end()) {
        return;
    }
    stdinput.push_back(name);
}

void options_class::add_stdoutput(const std::string &name) {
    if (std::find(stdoutput.begin(), stdoutput.end(), name) != stdoutput.end()) {
        return;
    }
    stdoutput.push_back(name);
}
void options_class::add_stderror(const std::string &name) {
    if (std::find(stderror.begin(), stderror.end(), name) != stderror.end()) {
        return;
    }
    stderror.push_back(name);
}