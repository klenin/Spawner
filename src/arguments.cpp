#include "arguments.h"
#include <iostream>


void dictionary_c::add_value(const std::string &key, temp_name &value) {
    if (!key.length()) {
            return;
    }
    if (contents.find(key) != contents.end()) {
        throw "";
    }
    contents[key] = value;
}


std::pair<std::string, std::string> extract_argument(const std::string &s) {
    std::map<std::string, bool> dividers;
    std::string left, right;
    size_t i = 0;
    dividers["="] = true;
    dividers[":"] = true;
    while (dividers.find(s.substr(i, 1)) == dividers.end() && i < s.length()) ++i;
    left = s.substr(0, i);
    if (s.length() > i) {
        right = s.substr(i + 1, s.length() - i);
    }
    return std::pair<std::string, std::string>(left, right);
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
    for (auto parser = parsers.begin(); parser != parsers.end(); parser++) {
        (*parser).invoke_initialization(*this);
    }
    while (current_position() < argc) {
        fetch_current_position();
        if (system_parse()) {
            //save_current_position(&system_parser);
        }
        for (auto parser = parsers.begin(); parser != parsers.end(); parser++) {
            fetch_current_position();
            if ((*parser).parse(*this)) {
                save_current_position(&(*parser));
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