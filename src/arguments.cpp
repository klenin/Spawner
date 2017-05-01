#include "arguments.h"

#include <iostream>

#include <stdarg.h>

compact_list_c::compact_list_c(){}

compact_list_c::compact_list_c(int dummy_value, ...) {
    va_list vl;
    char *value;
    va_start(vl, dummy_value);
    do {
        value = va_arg(vl, char*);
        if (value) {
            items.push_back(value);
        }
    } while (value);
    va_end(vl);
}
size_t compact_list_c::size() const {
    return items.size();
}
std::vector<std::string> compact_list_c::vector() const {
    return items;
}
std::string compact_list_c::operator[] (size_t index) {
    return items[index];
}
compact_list_c::operator std::vector<std::string>() const {
    return vector();
}

abstract_parser_c::~abstract_parser_c() {
    for (const auto& i : parameters) {
        i.second->dereference();
    }
}

abstract_argument_parser_c *abstract_parser_c::add_argument_parser(const std::vector<std::string> &params, abstract_argument_parser_c *argument_parser) {
    for (const auto& i : params) {
        parameters[i] = argument_parser->reference();
    }
    parsers[argument_parser] = params;
    return argument_parser;
}

bool settings_parser_c::is_program() {
    return (position < arg_c && arg_v[position][0] != '-');
}

void settings_parser_c::parse_program() {
    program = get_next_argument();
    while (position < arg_c && arg_v[position] != separator) {
        program_arguments.push_back(get_next_argument());
    }
}

void settings_parser_c::add_parser(abstract_parser_c *parser) {
    parsers.push_back(parser);
    if (position) {
        parser->invoke_initialization(*this);
    }
}

void settings_parser_c::set_dividers(const std::vector<std::string> &d) {
    dividers.clear();
    dividers.insert(dividers.begin(), d.begin(), d.end());
}

settings_parser_c::settings_parser_c()
    : position(0)
    , stopped(0)
    , arg_c(0)
    , arg_v(nullptr)
{}
int settings_parser_c::current_position() {
    return position;
}
size_t settings_parser_c::saved_count() {
    return saved_positions.size();
}
void settings_parser_c::stop() {
    stopped = true;
}
const char *settings_parser_c::get_next_argument() {
    if (position >= arg_c) {
        return nullptr;
    }
    if (arg_v[position] == separator) {
        position++;
        return SEPARATOR_ARGUMENT;
    }
    return arg_v[position++];
}
void settings_parser_c::save_current_position(abstract_parser_c *associated_parser) {
    saved_positions.push_back(std::pair<int, abstract_parser_c*>(current_position(), associated_parser));
}
abstract_parser_c *settings_parser_c::pop_saved_parser() {
    auto value = saved_positions.front();//std::pair<int, void*>
    position = value.first;
    saved_positions.erase(saved_positions.begin());
    return value.second;
}
void settings_parser_c::clear_parsers() {
    for (auto p : parsers) {
        delete p;
    }
    parsers.clear();
}
void settings_parser_c::clear_program_parser() {
    program.clear();
    program_arguments.clear();
}

settings_parser_c::~settings_parser_c() {
    clear_parsers();
}

std::string settings_parser_c::get_program() {
    return program;
}
std::vector<std::string> settings_parser_c::get_program_arguments() {
    return program_arguments;
}
void settings_parser_c::set_separator(const std::string &s) {
    separator = long_arg(s);
}
bool settings_parser_c::parse(int argc, char *argv[]) {
    arg_c = argc;
    arg_v = argv;
    position = 1;
    for (auto p : parsers) {
        p->invoke_initialization(*this);
    }

    while (current_position() < argc && !stopped) {
        for (auto parser : parsers) {
            int fetched_position = position;
            if (parser->parse(*this)) {
                save_current_position(parser);
            }
            position = fetched_position;
        }
        if (saved_count() == 1) {
            try {
                pop_saved_parser()->invoke(*this);
            } catch (std::string &error) {
                std::cerr << "Invalid parameter value: " << arg_v[position - 1] << " with error: " << error << std::endl;
                return false;
            }
        } else if (saved_count() > 1) {
            //ambiguous arguments
            //throw
            std::cerr << "ambiguous arguments" << std::endl;
        } else {
            if (is_program()) {
                parse_program();
                //parse_program//until separator detected
            } else {
                //unknown_argument
                std::cerr << "unknown argument " << arg_v[position] << std::endl;
                //throw "";
                return false;
            }
        }
    }
    return true;
}
size_t settings_parser_c::parsers_count() {
    return parsers.size();
}

void settings_parser_c::pop_back() {
    if (!parsers_count()) {
        return;
    }
    parsers.pop_back();
}

std::string settings_parser_c::help() {
    std::string result;
    for (auto parser : parsers) {
        result += parser->help(*this);
    }
    return result;
}

console_argument_parser_c::console_argument_parser_c()
    : abstract_parser_c()
    , last_state(argument_ok_state)
    , parser_object(nullptr)
{}
bool console_argument_parser_c::parse(abstract_settings_parser_c &parser_object) {
    console_argument_parser_c::parser_object = &parser_object;
    last_state = argument_error_state;
    if (process_argument(parser_object.get_next_argument()) == argument_error_state) {
        return false;
    }
    while (last_state == argument_started_state) {
        process_value(parser_object.get_next_argument());
    }
    return last_state == argument_ok_state;
}
abstract_argument_parser_c *console_argument_parser_c::add_flag_parser(const std::vector<std::string> &params, abstract_argument_parser_c *argument_parser) {
    for (const auto& i : params) {
        is_flag[i] = true;
    }
    return add_argument_parser(params, argument_parser);
}
console_argument_parser_c::parsing_state_e console_argument_parser_c::process_argument(const char *argument) {
    //check if in the dictionary
    if (!argument) {
        return last_state = argument_error_state;
    }
    std::string s = argument;
    std::string left, right;
    if (is_flag.find(std::string(argument)) != is_flag.end()) {
        value = "1";
        current_argument_parser = parameters[argument];
        last_state = argument_ok_state;
        return last_state;
    }
    size_t length = s.length();
    size_t divider_length = 0;
    for (const auto& i : parser_object->dividers) {
        size_t pos = s.find(i);
        length = min_def(length, pos);
        if (length == pos) {
            divider_length = max_def(divider_length, i.length());
        }
    }
    left = s.substr(0, length);
    //check left
    if (parameters.find(left) == parameters.end()) {
        return argument_error_state;
    } else if (is_flag.find(left) != is_flag.end()) {
        return argument_error_state;
    }
    current_argument_parser = parameters[left];
    last_state = argument_started_state;
    if (length < s.length()) {
        right = s.substr(length + divider_length, s.length() - length - divider_length);
        return process_value(right.c_str());
    }
    return last_state;
}
console_argument_parser_c::parsing_state_e console_argument_parser_c::process_value(const char *argument) {
    if (last_state == argument_ok_state) {
        return last_state;
    }
    if (!argument) {
        return last_state = argument_error_state;
    }
    value = argument;
    last_state = argument_ok_state;
    return last_state;
}
bool console_argument_parser_c::invoke(abstract_settings_parser_c &parser_object) {
    return current_argument_parser->apply(value);
}

bool console_argument_parser_c::invoke_initialization(abstract_settings_parser_c &parser_object) {
    initialized = true;
    return true;
}

std::string console_argument_parser_c::help(const abstract_settings_parser_c &parser) {
    std::ostringstream res;
    std::string dividers = " ";
    if (parser.dividers.size()) {
        dividers = parser.dividers[0];
    }
    for (auto i : parsers) {
        //iterating over all arguments
        for (auto j = i.second.begin(); j != i.second.end(); ++j) {
            if (j != i.second.begin()) {
                res << ", ";
            }
            if (*j == SEPARATOR_ARGUMENT) {
                res << "--<separator>";
            } else {
                res << *j;
            }
            if (!is_flag[*j]) {
                res << dividers << i.first->value_description();
            }
        }
        res << std::endl << "\t" << i.first->description() << std::endl;
    }
    return res.str();
}

environment_variable_parser_c::environment_variable_parser_c() : abstract_parser_c() {
}

bool environment_variable_parser_c::invoke_initialization(abstract_settings_parser_c &parser_object) {
    static char buffer[4096];
    if (initialized) {
        return true;
    }

    for (const auto& i : parameters) {
        auto result = get_env_var(i.first.c_str(), buffer, sizeof(buffer));

        if (result > sizeof(buffer)) {
            std::cerr << "Invalid parameter value for \"" << i.first << "\" with error: Buffer overflow" << std::endl;
        } else if (result > 0) {
            try {
                i.second->apply(std::string(buffer, result));
            } catch (std::string &error) {
                std::cerr << "Invalid parameter value for \"" << i.first << "\" with error: " << error << std::endl;
            }
        }
    }
    initialized = true;
    return true;
}

std::string environment_variable_parser_c::help(abstract_settings_parser_c *parser) {
    std::ostringstream res;
    for (auto i : parsers) {
        //iterating over all arguments
        for (auto j = i.second.begin(); j != i.second.end(); ++j) {
            if (j != i.second.begin()) {
                res << ", ";
            }
            res << *j << "=" << i.first->value_description();
        }
        res << std::endl << "\t" << i.first->description() << std::endl;
    }
    return res.str();
}
