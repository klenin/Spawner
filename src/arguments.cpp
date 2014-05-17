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

void argument_parser_c::add_dictionary(temp_name *dictionary) {
    while ((*dictionary).callback) {
        short_names.add_value((*dictionary).short_name, *dictionary);
        long_names.add_value((*dictionary).long_name, *dictionary);
        environment_names.add_value((*dictionary).environment_name, *dictionary);
        dictionary++;
    }
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

void argument_parser_c::parse(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        std::cout << argument << std::endl;
        if (argument[0] == '-') {
            if (argument.length() > 1) {
                if (argument[1] == '-') {
                    //long argument
                    std::pair<std::string, std::string> arg = extract_argument(argument.substr(2, argument.length() - 1));
                    std::cout << "\"" << arg.first << "\" = \"" << arg.second << "\"" << std::endl;
                    std::cout << "long argument" << std::endl;
                } else {
                    //short argument
                    std::pair<std::string, std::string> arg = extract_argument(argument.substr(1, argument.length() - 1));
                    std::cout << "\"" << arg.first << "\" = \"" << arg.second << "\"" << std::endl;
                    std::cout << "short argument" << std::endl;
                }
            } else {
                //strange symbol
                std::cout << "shit" << std::endl;
            }
        } else {
            //
            std::cout << "unknown" << std::endl;
        }
    }
}