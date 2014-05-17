#ifndef _SPAWNER_ARGUMENTS_
#define _SPAWNER_ARGUMENTS_

#include <algorithm>
#include <string>
#include <map>
#include <vector>

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


//configuration_manager
class argument_parser_c {
private:
    dictionary_c short_names;
    dictionary_c long_names;
    dictionary_c environment_names;
public:
    template<typename T> static argument_value_c *create_value(const std::string &value) {
        return new T(value);
    }

    void add_dictionary(temp_name *dictionary);
    void parse(int argc, char *argv[]);
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

struct argument_t {
    char *name;
    argument_value_c *(*callback)(const std::string&);
    value_type_e value_type;
    behavior_type_e behavior;
    char *help;
};

#define VALUE_CONSTRUCTOR(X) &argument_parser_c::create_value<X>

argument_t base_dictionary[] = {
    {
        "output_stream",
        VALUE_CONSTRUCTOR(argument_stream_value),
        string_value, //expected value
        array_behavior,
        "help"
    }
};
    //&argument_parser::set_value<output_stream_value_c>,




class abstract_parser_c {
public:
    virtual bool invoke_initialization(parser_object){return false;} //init settings for parser object
    virtual bool invoke(parser_object) {return false;}
    virtual std::string help() { return ""; }
};

class console_argument_parser_c : public abstract_parser_c {
public:
    virtual bool invoke(parser_object) {
        while (process(parser_object.get_next_argument()));
    }
    virtual std::string help() { return ""; }
};

class environment_variable_parser_c : public abstract_parser_c {
protected:
    std::map<argument_type, std::string> values;
public:
    environment_variable_parser_c(dictionary) {
        foreach (item in dictionary) {
            if (exists_environment_variable(item.name)) {
                values[item.type] = get_environment_variable(item.name);
            }
        }
    }
    virtual bool invoke_initialization(parser_object){
        return true;
    }
};

parse () {
    while (arguments_exists) {
        bool already_parsed = system_parse();
        foreach (parser in parsers) {
            fetch_position();
            bool parsed = parser.invoke(this);
            if (parsed && already_parsed) {
                conflict;
            }
            already_parsed = parsed;
        }
        if (!already_parsed) {
            if (is_program) {
                parse_program
            } else {
                unknown_argument
            }
        }
    }
}




#endif//_SPAWNER_ARGUMENTS_