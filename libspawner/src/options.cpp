#include "options.h"

#include <algorithm>
#include <iterator>

// TODO: use c_list /dev/null || nul system dependent
const std::string CLEAR_STRING = "nul";

void options_class::add_argument(std::string argument) {
    arguments.push_back(argument);
}

void options_class::add_arguments(const std::vector<std::string> &arguments_a) {
    for (const auto& i : arguments_a) {
        arguments.push_back(i);
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

std::string options_class::get_argument(const size_t index) const {
    if (index < arguments.size()) {
        return *std::next(arguments.cbegin(), index);
    }
    return ""; //TODO: raise an error
}

size_t options_class::get_arguments_count() const {
    return arguments.size();
}

std::string options_class::format_arguments() const
{
    std::string arguments_string = get_arguments();
    if (arguments_string.empty()) {
        return "<none>";
    }
    return arguments_string;
}

//TODO: rethink this
void options_class::add_stdinput(const std::string &name) {
    //check if file exists
    if (std::find(stdinput.begin(), stdinput.end(), name) != stdinput.end()) {
        return;
    }
    if (name == CLEAR_STRING || name.empty()) {
        stdinput.clear();
        return;
    }
    stdinput.push_back(name);
}

void options_class::add_stdoutput(const std::string &name) {
    if (std::find(stdoutput.begin(), stdoutput.end(), name) != stdoutput.end()) {
        return;
    }
    if (name == CLEAR_STRING || name.empty()) {
        stdoutput.clear();
        return;
    }
    stdoutput.push_back(name);
}

void options_class::add_stderror(const std::string &name) {
    if (std::find(stderror.begin(), stderror.end(), name) != stderror.end()) {
        return;
    }
    if (name == CLEAR_STRING || name.empty()) {
        stderror.clear();
        return;
    }
    stderror.push_back(name);
}

void options_class::add_environment_variable(const std::string &envStr) {
    std::string name, val;

    int pos = find(envStr.begin(), envStr.end(), '=') - envStr.begin();
    int l = envStr.length();

    if (pos == 0 || pos == l) {
        return; // TODO: raise error
    }

    name = envStr.substr(0, pos);
    val = envStr.substr(pos + 1);

    environmentVars.push_back(std::make_pair(name, val));
}

//TODO: rethink this
void options_class::clear_stdinput() {
    stdinput.clear();
}

void options_class::clear_stdoutput() {
    stdoutput.clear();
}

void options_class::clear_stderror() {
    stderror.clear();
}
