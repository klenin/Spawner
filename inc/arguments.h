#ifndef _SPAWNER_ARGUMENTS_
#define _SPAWNER_ARGUMENTS_

//#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <json/json.h>
#include <inc/compatibility.h>
#include <functional>
#define min_def(x, y)((x)<(y)?(x):(y))
#define max_def(x, y)((x)>(y)?(x):(y))

#define ARGUMENT() 

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
    operator std::vector<std::string>() const;
};

#define c_lst(...) compact_list_c(0, ##__VA_ARGS__, NULL)

class abstract_settings_parser_c {
public:
    std::vector<std::string> dividers;
    virtual char *get_next_argument() = 0;
//    virtual void set_value(argument_type_t argument, const std::string &value) = 0;
//    virtual void set_init_value(argument_type_t argument, const std::string &value) = 0;
};

class abstract_parser_c {
protected:
    std::map<std::string, std::function<bool(const std::string&)> > parameters;
    std::function<bool(const std::string&)> callback;
public:
    //abstract_parser_c(const parser_t &parser){}
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object){return false;} //init settings for parser object
    virtual void add_parameter(const std::vector<std::string> &params, std::function<bool(const std::string)> callback);
    virtual bool parse(abstract_settings_parser_c &parser_object) {return false;}
    virtual bool invoke(abstract_settings_parser_c &parser_object) {return false;}
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

#define short_arg(x) ((std::string("-")+x).c_str())
#define long_arg(x) ((std::string("--")+x).c_str())


//configuration_manager
class settings_parser_c: public abstract_settings_parser_c {
private:
    int position;
    int fetched_position;
    bool stopped;

    std::vector<std::pair<int, abstract_parser_c*> > saved_positions;
    std::vector<abstract_parser_c*> parsers;
    std::string separator;
    std::string program;
    std::vector<std::string> program_arguments;
    int arg_c;
    char **arg_v;
    bool is_program();
    void parse_program();
public:
    settings_parser_c();
    ~settings_parser_c();

    void add_parser(abstract_parser_c *parser);
    void set_dividers(const std::vector<std::string> &d);

    void clear_parsers();

    //void register_constructors(parser_constructor_t *parser_constructors);

    int current_position();
    void fetch_current_position();
    void restore_position();
    size_t saved_count();
    void stop();

    void save_current_position(abstract_parser_c *associated_parser);
    abstract_parser_c *pop_saved_parser();

    char *get_next_argument();
    std::string get_program();
    std::vector<std::string> get_program_arguments();
    void set_separator(const std::string &s);
    void reset_program();

    void parse(int argc, char *argv[]);

    size_t parsers_count();
    void pop_back();
};




class console_argument_parser_c : public abstract_parser_c {
protected:
    enum parsing_state_e {
        argument_started_state,
        argument_ok_state,
        argument_error_state,
    };
    std::map<std::string, bool> is_flag;
    std::string argument;
    std::string value;
    argument_type_t argument_name;
    parsing_state_e last_state;
    std::map<std::string, console_argument_t> symbol_table;
    abstract_settings_parser_c *parser_object;
public:
    console_argument_parser_c();
    void set_flag(const std::vector<std::string> &v);
    virtual bool parse(abstract_settings_parser_c &parser_object);
    virtual parsing_state_e process_argument(const char *argument);
    virtual parsing_state_e process_value(const char *argument);
    virtual bool invoke(abstract_settings_parser_c &parser_object);
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
    environment_variable_parser_c();
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object);
};











#endif//_SPAWNER_ARGUMENTS_