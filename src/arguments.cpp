#include "arguments.h"
#include <iostream>

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
}
size_t compact_list_c::size() const {
    return items.size();
}
std::string compact_list_c::operator[] (size_t index) {
    return items[index];
}


void settings_parser_c::add_parser(const std::string &parser) {
    if (registered_parser_constructors.find(registered_parsers[parser].type) != registered_parser_constructors.end()) {
        parsers.push_back((registered_parser_constructors[registered_parsers[parser].type].constructor)(registered_parsers[parser]));
    }
}
void settings_parser_c::rebuild_parsers() {
    parsers.clear();//not good
    for (auto i = enabled_dictionaries.begin(); i != enabled_dictionaries.end(); i++) {
        if ((*i).second) {
            dictionary_t dictionary = registered_dictionaries[(*i).first];
            for (int k = 0; k < dictionary.parsers.size(); ++k) {
                add_parser(dictionary.parsers[k]);
            }
        }
    }
}

settings_parser_c::settings_parser_c() : position(0) {}

void settings_parser_c::register_dictionaries(dictionary_t *dictionaries) {
    while (dictionaries && dictionaries->name) {
        register_dictionary(*dictionaries);
        dictionaries++;
    }
}
void settings_parser_c::register_dictionary(const dictionary_t &dictionary) {
    registered_dictionaries[dictionary.name] = dictionary;
}
void settings_parser_c::register_parsers(parser_t *parsers) {
    while (parsers && parsers->name) {
        registered_parsers[parsers->name] = *parsers;
        parsers++;
    }
}
void settings_parser_c::clear_dictionaries() {
}
void settings_parser_c::enable_dictionary(const std::string &name) {
    if (registered_dictionaries.find(name) != registered_dictionaries.end()) {
        enabled_dictionaries[name] = true;
        rebuild_parsers();
    }
}
void settings_parser_c::register_constructors(parser_constructor_t *parser_constructors) {

}

int settings_parser_c::current_position() {
    return position;
}
void settings_parser_c::fetch_current_position() {
    fetched_position = position;
}
void settings_parser_c::restore_position() {
    position = fetched_position;
}
bool settings_parser_c::system_parse() {
    return false;
}
size_t settings_parser_c::saved_count() {
    return saved_positions.size();
}
char *settings_parser_c::get_next_argument() {
    if (position >= arg_c) {
        return NULL;
    }
    return arg_v[position];
}
void settings_parser_c::save_current_position(abstract_parser_c *associated_parser) {
    saved_positions.push_back(std::pair<int, abstract_parser_c*>(current_position(), associated_parser));
}
abstract_parser_c *settings_parser_c::pop_saved_parser() {
    auto value = saved_positions.front();//std::pair<int, void*>
    position = value.first;
    saved_positions.erase(saved_positions.begin(), saved_positions.begin());
    return value.second;
}


void settings_parser_c::parse(int argc, char *argv[]) {
    arg_c = argc;
    arg_v = argv;
    position = 1;
    for (auto parser = parsers.begin(); parser != parsers.end(); parser++) {
        (*parser)->invoke_initialization(*this);
    }
    while (current_position() < argc) {
        for (auto parser = parsers.begin(); parser != parsers.end(); parser++) {
            fetch_current_position();
            if ((*parser)->parse(*this)) {
                save_current_position((*parser));
            }
            restore_position();
        }
        if (saved_count() == 1) {
            pop_saved_parser()->invoke(*this);
        } else if (saved_count() > 1) {
            //ambiguous arguments
            //throw
        } else {
            if (false) {//is_program()) {
                //parse_program//until separator detected
            } else {
                //unknown_argument
                //throw
            }
        }
    }
}







console_argument_parser_c::console_argument_parser_c(const parser_t &parser) {}
bool console_argument_parser_c::parse(abstract_settings_parser_c &parser_object) {
    if (process_argument(parser_object.get_next_argument()) == argument_error_state) {
        return false;
    }
    while (process_value(parser_object.get_next_argument()) == argument_started_state);
    return last_state == argument_ok_state;
}
console_argument_parser_c::parsing_state_e console_argument_parser_c::process_argument(const char *argument) {
    //check if in the dictionary
    if (!argument) {
        return last_state = argument_error_state;
    }
    std::vector<std::string> dividers;
    std::string s = argument;
    std::string left, right;
    size_t length = s.length();
    size_t divider_length = 0;
    dividers.push_back("=");
    dividers.push_back(":");
    for (auto i = dividers.begin(); i != dividers.end(); i++) {
        size_t pos = s.find(*i);
        length = min_def(length, pos);
        if (length == pos) {
            divider_length = max_def(divider_length, (*i).length());
        }
    }
    left = s.substr(0, length);
    //check left
    last_state = argument_started_state;
    if (length < s.length()) {
        right = s.substr(length + divider_length, s.length() - length - divider_length - 1);
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
    return last_state;
}
void console_argument_parser_c::invoke(abstract_settings_parser_c &parser_object) {
}
std::string console_argument_parser_c::help() { return ""; }



environment_variable_parser_c::environment_variable_parser_c(const parser_t &parser){}
environment_variable_parser_c::environment_variable_parser_c(std::vector<environment_desc_t> dictionary) {
    for (auto item = dictionary.begin(); item != dictionary.end(); item++) {
        if (exists_environment_variable((*item).name)) {
            values[(*item).type] = get_environment_variable((*item).name);
        }
    }
}
bool environment_variable_parser_c::exists_environment_variable(char *variable) {
    last_variable_name = variable;
    return GetEnvironmentVariable(variable, buffer, sizeof(buffer));
}
std::string environment_variable_parser_c::get_environment_variable(char *variable) {
    if (last_variable_name != variable) {
        GetEnvironmentVariable(variable, buffer, sizeof(buffer));
    }
    return buffer;
}
bool environment_variable_parser_c::invoke_initialization(abstract_settings_parser_c &parser_object){
    return true;
}