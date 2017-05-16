#include "options.h"

#include <algorithm>
#include <iterator>
#include <regex>

// TODO: use c_list /dev/null || nul system dependent
const std::string CLEAR_STRING = "*null";
const options_class::redirect_flags options_class::file_default = { false, false };
const options_class::redirect_flags options_class::pipe_default = { true, false };

void options_class::redirect_flags::apply(const redirect_flags& src) {
    flush = src.flush;
    exclusive = src.exclusive;
}

void options_class::add_argument(std::string argument) {
    arguments.push_back(argument);
}

void options_class::add_arguments(const std::vector<std::string> &arguments_a) {
    arguments.insert(arguments.cend(), arguments_a.cbegin(), arguments_a.cend());
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

void options_class::parse_flags(std::string flags_string, const redirect_flags &global_flags, const redirect_flags &default_flags, redirect_flags &dst_flags) const {
    if (flags_string.empty()) {
        dst_flags.apply(default_flags);
        return;
    }

    auto result = global_flags;

    auto iter = flags_string.cbegin();
    std::regex flags_regex("(-)?(f|e)", std::regex_constants::extended);
    std::smatch flags_match;

    while (std::regex_search(iter, flags_string.cend(), flags_match, flags_regex)) {
        auto value = flags_match[1].matched ? false : true;
        if (flags_match[2] == "f") {
            result.flush = value;
        }
        else if (flags_match[2] == "e") {
            result.exclusive = value;
        }
        else {
            PANIC((std::string("Bad flag: ") + std::string(flags_match[0])).c_str());
        }
        iter = flags_match[0].second;
    }
    dst_flags.apply(result);
}

bool options_class::parse_redirect(std::string redirect_string, redirect_flags &global_flags, redirect &redirect_result) const {
    redirect_result.original = redirect_string;
    if (redirect_string[0] == '*') {
        std::regex full_regex("^\\*((.*):)?((std)|((\\d+)\\.(std(in|out|err)))|(.*))?$");
        std::smatch full_match;
        if (std::regex_match(redirect_string, full_match, full_regex)) {
            // Group 1: flags
            // Group 2: flags without :
            // Group 3: redirect without flags
            // Group 4: redirect to std
            // Group 5: redirect to pipe
            // Group 6: redirect to pipe index
            // Group 7(8): redirect to pipe name(direction)
            // Group 9: filename
            PANIC_IF(!(full_match.size() > 9));
            if (full_match[9].matched) { // file redirects
                if (!full_match[1].matched) { // no have flags
                    PANIC((std::string("Bad redirect: ") + redirect_string).c_str());
                }
                if (full_match[9] == "") { // set global flags
                    parse_flags(full_match[2], global_flags, file_default, global_flags);
                    return false; // no save redirect
                }
                redirect_result.type = file;
                redirect_result.name = full_match[9];
                parse_flags(full_match[2], global_flags, file_default, redirect_result.flags);
            }
            else if (full_match[5].matched) { // pipe redirects
                redirect_result.type = pipe;
                redirect_result.name = full_match[7];
                redirect_result.pipe_index = stoi(full_match[6]);
                parse_flags(full_match[2], global_flags, pipe_default, redirect_result.flags);
            }
            else if (full_match[4].matched) { // std redirects
                redirect_result.type = std;
                parse_flags(full_match[2], global_flags, pipe_default, redirect_result.flags);
            }
            else {
                PANIC((std::string("Bad redirect: ") + redirect_string).c_str());
            }
        }
        else {
            PANIC((std::string("Bad redirect: ") + redirect_string).c_str());
        }
    }
    else {
        redirect_result.type = file;
        redirect_result.flags = file_default;
        redirect_result.name = redirect_string;
    }
    return true;
}

void options_class::add_std(std::vector<redirect> &dst, redirect_flags &global_flags, const std::string &redirect_str) const {
    for (const auto &stream : dst) {
        if (stream.original == redirect_str) {
            return;
        }
    }
    if (redirect_str == CLEAR_STRING || redirect_str.empty()) {
        dst.clear();
        return;
    }
    redirect stream_redirect;
    if (parse_redirect(redirect_str, global_flags, stream_redirect)) {
        for (auto &stream : dst) {
            if (stream.name == stream_redirect.name) {
                stream.flags.apply(stream_redirect.flags);
                return;
            }
        }
        dst.push_back(stream_redirect);
    }
}

void options_class::add_stdinput(const std::string &redirect_str) {
    add_std(stdinput, global_in_file_flags, redirect_str);
}

void options_class::add_stdoutput(const std::string &redirect_str) {
    add_std(stdoutput, global_out_file_flags, redirect_str);
}

void options_class::add_stderror(const std::string &redirect_str) {
    add_std(stderror, global_err_file_flags, redirect_str);
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
