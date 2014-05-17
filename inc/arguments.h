#ifndef _SPAWNER_ARGUMENTS_
#define _SPAWNER_ARGUMENTS_

#include <algorithm>
#include <string>
#include <map>
#include <vector>

#define min_def(x, y)((x)<(y)?(x):(y))
#define max_def(x, y)((x)>(y)?(x):(y))
class argument_value_c {
public:
    static argument_value_c *create_argument(const std::string &value) {
        return NULL;
    }
};

class argument_string_value_c : public argument_value_c{
private:
    std::string value;
public:
    argument_string_value_c(const std::string &value) : value(value) {}
};

struct temp_name;

class dictionary_c {
private:
    std::map<std::string, temp_name> contents;
public:
    void add_value(const std::string &key, temp_name &value);
};

enum on_error_behavior_e {
    on_error_skip,
    on_error_die
};

enum on_repeat_behavior_e {
    on_repeat_stack,
    on_repeat_replace,
    on_repeat_die
};

struct temp_name {
    char *short_name;
    char *long_name;
    char *environment_name;
    argument_value_c *(*callback)(const std::string&);
};

enum value_type_e {
    string_value,
    integer_value,
    real_value,
    bool_value,
};

enum behavior_type_e {
    array_behavior,
    variable_behavior
};

class abstract_settings_parser_c {
public:
    virtual char *get_next_argument() = 0;
};

class abstract_parser_c {
public:
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object){return false;} //init settings for parser object
    virtual bool parse(abstract_settings_parser_c &parser_object) {return false;}
    virtual void invoke(abstract_settings_parser_c &parser_object) {}
    virtual std::string help() { return ""; }
};

//configuration_manager
class settings_parser_c: public abstract_settings_parser_c {
private:
    int position;
    int fetched_position;
    void *system_parser;
    std::vector<std::pair<int, abstract_parser_c*> > saved_positions;
    std::vector<abstract_parser_c> parsers;
    int arg_c;
    char **arg_v;
    //std::vector<int> system_parsers;
public:
    settings_parser_c() {}
    template<typename T> static argument_value_c *create_value(const std::string &value) {
        return new T(value);
    }
    int current_position();
    void fetch_current_position();
    void restore_position();
    bool system_parse();
    size_t saved_count();
    char *get_next_argument();
    void save_current_position(abstract_parser_c *associated_parser);
    abstract_parser_c *pop_saved_parser();
    void parse(int argc, char *argv[]);
};


struct argument_t {
    char *name;
    argument_value_c *(*callback)(const std::string&);
    value_type_e value_type;
    behavior_type_e behavior;
    char *help;
};

#define VALUE_CONSTRUCTOR(X) &settings_parser_c::create_value<X>
typedef char *argument_type_t;

argument_t base_dictionary[] = {
    {
        "output_stream",
        VALUE_CONSTRUCTOR(argument_value_c),
        string_value, //expected value
        array_behavior,
        "help"
    }
};
    //&argument_parser::set_value<output_stream_value_c>,





class console_argument_parser_c : public abstract_parser_c {
private:
    enum parsing_state_e {
        argument_started_state,
        argument_ok_state,
        argument_error_state,
    };
    std::string argument;
    std::vector<std::string> values;
    char *argument_name;
    parsing_state_e last_state;
public:
    virtual bool parse(abstract_settings_parser_c &parser_object) {
        if (process_argument(parser_object.get_next_argument()) == argument_error_state) {
            return false;
        }
        while (process_value(parser_object.get_next_argument()) == argument_started_state);
        return last_state == argument_ok_state;
    }
    virtual parsing_state_e process_argument(const char *argument) {
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
    virtual parsing_state_e process_value(const char *argument) {
        if (last_state == argument_ok_state) {
            return last_state;
        }
        if (!argument) {
            return last_state = argument_error_state;
        }
        return last_state;
    }
    virtual void invoke(abstract_settings_parser_c &parser_object) {
    }
    virtual std::string help() { return ""; }
};

class environment_variable_parser_c : public abstract_parser_c {
protected:
    std::map<argument_type_t, std::string> values;
public:
    environment_variable_parser_c(dictionary) {
        foreach (item in dictionary) {
            if (exists_environment_variable(item.name)) {
                values[item.type] = get_environment_variable(item.name);
            }
        }
    }
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object){
        return true;
    }
};






#endif//_SPAWNER_ARGUMENTS_