#ifndef _SPAWNER_ARGUMENTS_
#define _SPAWNER_ARGUMENTS_

#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <json/json.h>
#include <inc/compatibility.h>

#define min_def(x, y)((x)<(y)?(x):(y))
#define max_def(x, y)((x)>(y)?(x):(y))

typedef char *argument_type_t;


//unique names for arguments
const argument_type_t sp_output_stream          = "output_stream";
const argument_type_t sp_input_stream           = "intput_stream";
const argument_type_t sp_error_stream           = "error_stream";

const argument_type_t sp_test                   = "test";

const argument_type_t sp_end                    = NULL;
const argument_type_t sp_reserved               = NULL;

typedef char *parser_type_t;

const parser_type_t sp_console_parser       = "console_parser";
const parser_type_t sp_environment_parser   = "environment_parser";

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
    compact_list_c();
    compact_list_c(int dummy_value, ...);
    size_t size() const;
    std::vector<std::string> vector() const;
    std::string operator[] (size_t index);
};

#define c_lst(...) compact_list_c(0, ##__VA_ARGS__, NULL)

class abstract_settings_parser_c {
public:
    virtual char *get_next_argument() = 0;
    virtual void set_value(argument_type_t argument, const std::string &value) = 0;
    virtual void set_init_value(argument_type_t argument, const std::string &value) = 0;
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
    parser_type_t type;
};

struct parser_constructor_t {
    parser_type_t type;
    abstract_parser_c *(*constructor)(const parser_t&);
};


struct console_argument_parser_settings_t {
    compact_list_c dividers;
};

enum console_argument_kind_t {
    console_argument_none,
    console_argument_required,
    console_argument_optional,
    console_argument_list,
};
struct console_argument_t {
    argument_type_t type;
    console_argument_kind_t kind;
    compact_list_c arguments;
    on_error_behavior_e on_error_behavior;
    on_repeat_behavior_e on_repeat_behavior;
};

struct environment_argument_t {
    argument_type_t type;
    compact_list_c names;
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
    std::map<std::string, parser_constructor_t> registered_parser_constructors;
    std::map<std::string, dictionary_t> registered_dictionaries;
    std::map<std::string, parser_t> registered_parsers;

    std::map<std::string, bool> enabled_dictionaries;

    std::vector<std::pair<int, abstract_parser_c*> > saved_positions;
    std::vector<abstract_parser_c*> parsers;
    int arg_c;
    char **arg_v;
    //std::vector<int> system_parsers;
    void add_parser(const std::string &parser);
    void rebuild_parsers();
public:
    Json::Value object, *current_task;
    settings_parser_c();
    template<typename T> static abstract_parser_c *create_parser(const parser_t &value) {
        return new T(value);
    }

    void register_dictionaries(dictionary_t *dictionaries);
    void register_dictionary(const dictionary_t &dictionary);
    void register_parsers(parser_t *parsers);
    void register_constructors(parser_constructor_t *parser_constructors);


    void clear_dictionaries();
    void enable_dictionary(const std::string &name);

    int current_position();
    void fetch_current_position();
    void restore_position();
    size_t saved_count();

    void save_current_position(abstract_parser_c *associated_parser);
    abstract_parser_c *pop_saved_parser();

    virtual void set_value(argument_type_t argument, const std::string &value);
    virtual void set_init_value(argument_type_t argument, const std::string &value);

    char *get_next_argument();

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
    argument_type_t argument_name;
    parsing_state_e last_state;
    std::vector<std::string> dividers;
    std::map<std::string, console_argument_t> symbol_table;
public:
    console_argument_parser_c(const parser_t &parser);
    virtual bool parse(abstract_settings_parser_c &parser_object);
    virtual parsing_state_e process_argument(const char *argument);
    virtual parsing_state_e process_value(const char *argument);
    virtual void invoke(abstract_settings_parser_c &parser_object);
    virtual std::string help();
};

class environment_variable_parser_c : public abstract_parser_c {
protected:
    std::map<argument_type_t, std::string> values;
    std::string last_variable_name;
    char buffer[4096];
    bool exists_environment_variable(const std::string &variable);
    std::string get_environment_variable(const std::string &variable);
public:
    environment_variable_parser_c(const parser_t &parser);
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object);
};










const console_argument_parser_settings_t default_console_parser_settings = {
    c_lst("=")
};
const console_argument_parser_settings_t spawner_console_parser_settings = {
    c_lst(":")
};
const console_argument_t system_arguments[] = {
    {
        sp_test,
        console_argument_required,
        c_lst(short_arg("t"), long_arg("test")),
        default_behavior
    },
    {
        sp_output_stream,
        console_argument_required,
        c_lst(short_arg("o"), long_arg("out")),
        default_behavior
    },
    {
        sp_input_stream,
        console_argument_required,
        c_lst(short_arg("i"), long_arg("in")),
        default_behavior
    },
    {
        sp_error_stream,
        console_argument_required,
        c_lst(short_arg("e"), long_arg("err")),
        default_behavior
    },
    {
        NULL
    }
};

static environment_argument_t system_environment[] = {
    {
        sp_test,
        c_lst("SP_TEST")
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
        sp_console_parser
    },
    {
        "system_environment",
        NULL,
        (void*)&system_environment,
        sp_environment_parser
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

static parser_constructor_t c_parser_constructors[] = {
    {
        sp_console_parser,
        PARSER_CONSTRUCTOR(console_argument_parser_c)
    },
    {
        sp_environment_parser,
        PARSER_CONSTRUCTOR(environment_variable_parser_c)
    },
    {
        NULL
    }
};

#endif//_SPAWNER_ARGUMENTS_