#ifndef _SPAWNER_ARGUMENTS_
#define _SPAWNER_ARGUMENTS_

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <inc/compatibility.h>

#define min_def(x, y)((x)<(y)?(x):(y))
#define max_def(x, y)((x)>(y)?(x):(y))

typedef char *argument_type_t;


//unique names for arguments
const argument_type_t sp_output_stream          = "output_stream";
const argument_type_t sp_input_stream           = "intput_stream";
const argument_type_t sp_error_stream           = "error_stream";

const argument_type_t sp_end                    = NULL;
const argument_type_t sp_reserved               = NULL;

enum on_error_behavior_e {
    on_error_skip,
    on_error_die
};

enum on_repeat_behavior_e {
    on_repeat_stack,
    on_repeat_replace,
    on_repeat_die
};

class compact_list_c {
protected:
    std::vector<std::string> items;
public:
    compact_list_c(){}
    compact_list_c(int dummy_value, ...) {
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
    size_t size() const {
        return items.size();
    }
    std::string operator[] (size_t index) {
        return items[index];
    }
};

#define c_lst(...) compact_list_c(0, ##__VA_ARGS__, NULL)

class abstract_settings_parser_c {
public:
    virtual char *get_next_argument() = 0;
};

class abstract_parser_c {
public:
    //abstract_parser_c(const parser_t &parser){}
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object){return false;} //init settings for parser object
    virtual bool parse(abstract_settings_parser_c &parser_object) {return false;}
    virtual void invoke(abstract_settings_parser_c &parser_object) {}
    virtual std::string help() { return ""; }
};

struct parser_t {
    char *name;
    void *settings;
    void *items;
    abstract_parser_c *(*callback)(const parser_t&);
};

struct console_parser_settings_t {
    compact_list_c dividers;
};

struct console_argument_c {
    argument_type_t type;
    compact_list_c arguments;
    on_error_behavior_e on_error_behavior;
    on_repeat_behavior_e on_repeat_behavior;
};

#define default_behavior on_error_die, on_repeat_replace
#define short_arg(x) ((std::string("-")+x).c_str())
#define long_arg(x) ((std::string("--")+x).c_str())




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

struct dictionary_t {
    char *name;
    char *description;
    compact_list_c parsers;
};


//configuration_manager
class settings_parser_c: public abstract_settings_parser_c {
private:
    int position;
    int fetched_position;
    void *system_parser;
    std::map<std::string, dictionary_t> registered_dictionaries;
    std::map<std::string, parser_t> registered_parsers;
    std::map<std::string, bool> enabled_dictionaries;
    std::vector<std::pair<int, abstract_parser_c*> > saved_positions;
    std::vector<abstract_parser_c*> parsers;
    int arg_c;
    char **arg_v;
    //std::vector<int> system_parsers;
    void add_parser(const std::string &parser) {
        if (registered_parsers[parser].callback) {
            parsers.push_back((registered_parsers[parser].callback)(registered_parsers[parser]));
        }
    }
    void rebuild_parsers() {
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
public:
    settings_parser_c() : position(0) {}
    template<typename T> static abstract_parser_c *create_parser(const parser_t &value) {
        return new T(value);
    }
    void register_dictionaries(dictionary_t *dictionaries) {
        while (dictionaries && dictionaries->name) {
            register_dictionary(*dictionaries);
            dictionaries++;
        }
    }
    void register_dictionary(const dictionary_t &dictionary) {
        registered_dictionaries[dictionary.name] = dictionary;
    }
    void register_parsers(parser_t *parsers) {
        while (parsers && parsers->name) {
            registered_parsers[parsers->name] = *parsers;
            parsers++;
        }
    }
    void clear_dictionaries() {
    }
    void enable_dictionary(const std::string &name) {
        if (registered_dictionaries.find(name) != registered_dictionaries.end()) {
            enabled_dictionaries[name] = true;
            rebuild_parsers();
        }
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

#define PARSER_CONSTRUCTOR(X) &settings_parser_c::create_parser<X>

/*argument_t base_dictionary[] = {
    {
        "output_stream",
        VALUE_CONSTRUCTOR(argument_value_c),
        string_value, //expected value
        array_behavior,
        "help"
    }
};*/
    //&argument_parser::set_value<output_stream_value_c>,





class console_argument_parser_c : public abstract_parser_c {
private:
    enum parsing_state_e {
        argument_started_state,
        argument_ok_state,
        argument_error_state,
    };
    std::string argument;
    std::string value;
    char *argument_name;
    parsing_state_e last_state;
public:
    console_argument_parser_c(const parser_t &parser) {}
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

struct environment_desc_t {
    char *name;
    argument_type_t type;
};
class environment_variable_parser_c : public abstract_parser_c {
protected:
    std::map<argument_type_t, std::string> values;
    std::string last_variable_name;
    static char buffer[4096];
public:
    environment_variable_parser_c(const parser_t &parser){}
    environment_variable_parser_c(std::vector<environment_desc_t> dictionary) {
        for (auto item = dictionary.begin(); item != dictionary.end(); item++) {
            if (exists_environment_variable((*item).name)) {
                values[(*item).type] = get_environment_variable((*item).name);
            }
        }
    }
    bool exists_environment_variable(char *variable) {
        last_variable_name = variable;
        return GetEnvironmentVariable(variable, buffer, sizeof(buffer));
    }
    std::string get_environment_variable(char *variable) {
        if (last_variable_name != variable) {
            GetEnvironmentVariable(variable, buffer, sizeof(buffer));
        }
        return buffer;
    }
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object){
        return true;
    }
};










const console_parser_settings_t default_console_parser_settings = {
    c_lst("=")
};
const console_parser_settings_t spawner_console_parser_settings = {
    c_lst(":")
};
const console_argument_c system_arguments[] = {
    {
        sp_output_stream,
        c_lst(short_arg("o"), long_arg("out")),
        default_behavior
    },
    {
        sp_input_stream,
        c_lst(short_arg("i"), long_arg("in")),
        default_behavior
    },
    {
        sp_error_stream,
        c_lst(short_arg("e"), long_arg("err")),
        default_behavior
    },
    {
        NULL
    }
};

static parser_t c_parsers[] = {
    {
        "system",
        (void*)&default_console_parser_settings,
        (void*)&system_arguments,
        PARSER_CONSTRUCTOR(console_argument_parser_c)
    },
    {
        "system_environment",
        NULL
    },
    {
        "system_json",
        NULL
    },
    {
        NULL,
        NULL
    }
};

static dictionary_t c_dictionaries[] = {
    {
        "system",
        "Default system commands",
        c_lst("system", "system_environment", "system_json")
    },
    {
        NULL
    }
};



#endif//_SPAWNER_ARGUMENTS_