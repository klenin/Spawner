#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

#include "inc/uconvert.h"
#include "platform.h"

#define min_def(x, y)((x)<(y)?(x):(y))
#define max_def(x, y)((x)>(y)?(x):(y))

#define ARGUMENT()

typedef char *argument_type_t;

static const char *SEPARATOR_ARGUMENT = "spawner.argument.separator";

//unique names for arguments

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

#define c_lst(...) compact_list_c(0, ##__VA_ARGS__, nullptr)

class abstract_settings_parser_c {
public:
    std::vector<std::string> dividers;
    virtual const char *get_next_argument() = 0;
    virtual std::string help() = 0;
//    virtual void set_value(argument_type_t argument, const std::string &value) = 0;
//    virtual void set_init_value(argument_type_t argument, const std::string &value) = 0;
};

class argument_parser_exception_c {
private:
    std::string exception;
public:
    argument_parser_exception_c(const std::string &exception) : exception(exception) {}
};

class abstract_argument_parser_c {
friend class command_handler_c;
private:
    unsigned int reference_count;
protected:
    std::string default_error;
    std::string description_string;
public:
    abstract_argument_parser_c() : reference_count(0) {}
    virtual ~abstract_argument_parser_c() {}
    virtual void after() {}
    virtual bool set(const std::string &s) = 0;
    virtual bool apply(const std::string &s) = 0;
    void dereference() { reference_count--; if (!reference_count) delete this; }
    abstract_argument_parser_c *reference() { reference_count++; return this; }
    virtual std::string value_description() { return "<value>"; }
    std::string description() { return description_string; }
    abstract_argument_parser_c *set_description(const std::string &_description) { description_string = _description; return this; }
};

template<typename T>
class base_argument_parser_c : public abstract_argument_parser_c {
protected:
    T &value;
    std::string error;
public:
    base_argument_parser_c(T &value) : value(value), abstract_argument_parser_c() {}
    virtual bool set(const std::string &s) {return false;}
    virtual bool apply(const std::string &s) {
        if (!set(s)) {
            throw error;
        }
        after();
        return true;
    }
};

class abstract_parser_c {
protected:
    std::map<std::string, abstract_argument_parser_c*> parameters;
    std::map<abstract_argument_parser_c*, std::vector<std::string>> parsers;
    abstract_argument_parser_c *current_argument_parser;
    bool initialized;
public:
    //abstract_parser_c(const parser_t &parser) {}
    abstract_parser_c() : initialized(false), current_argument_parser(nullptr) {}
    ~abstract_parser_c();
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object) {return false;} //init settings for parser object
    virtual abstract_argument_parser_c *add_argument_parser(const std::vector<std::string> &params, abstract_argument_parser_c *argument_parser);
    virtual bool parse(abstract_settings_parser_c &parser_object) {return false;}
    virtual bool invoke(abstract_settings_parser_c &parser_object) {return false;}
    virtual std::string help(const abstract_settings_parser_c &parser) { return ""; }
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

    void clear_program_parser();

    //void register_constructors(parser_constructor_t *parser_constructors);

    int current_position();
    size_t saved_count();
    void stop();

    void save_current_position(abstract_parser_c *associated_parser);
    abstract_parser_c *pop_saved_parser();

    const char *get_next_argument();
    std::string get_program();
    std::vector<std::string> get_program_arguments();
    void set_separator(const std::string &s);

    bool parse(int argc, char *argv[]);

    size_t parsers_count();
    void pop_back();
    virtual std::string help();
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
    parsing_state_e last_state;
    std::map<std::string, console_argument_t> symbol_table;
    abstract_settings_parser_c *parser_object;
public:
    console_argument_parser_c();
    virtual abstract_argument_parser_c *add_flag_parser(const std::vector<std::string> &params, abstract_argument_parser_c *argument_parser);
    virtual bool parse(abstract_settings_parser_c &parser_object);
    virtual parsing_state_e process_argument(const char *argument);
    virtual parsing_state_e process_value(const char *argument);
    virtual bool invoke(abstract_settings_parser_c &parser_object);
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object);
    virtual std::string help(const abstract_settings_parser_c &parser);
};

class environment_variable_parser_c : public abstract_parser_c {
protected:
    std::map<argument_type_t, std::string> values;
    std::string last_variable_name;
public:
    environment_variable_parser_c();
    virtual bool invoke_initialization(abstract_settings_parser_c &parser_object);
    virtual std::string help(abstract_settings_parser_c *parser);
};

class string_argument_parser_c : public base_argument_parser_c < std::string > {
public:
    string_argument_parser_c(std::string &value) : base_argument_parser_c<std::string>(value) {}
    virtual bool set(const std::string &s) {
        value = s;
        return true;
    }
};

class environment_mode_argument_parser_c : public string_argument_parser_c {
public:
    environment_mode_argument_parser_c(std::string &mode) : string_argument_parser_c(mode) {}
    virtual std::string value_description() { return "inherit|user-default|clear"; }
    virtual bool set(const std::string &m) {
        std::vector<std::string> acceptables = {
            "inherit",
            "user-default",
            "clear",
        };

        if (std::find(acceptables.begin(), acceptables.end(), m) != acceptables.end()) {
            value = m;

            return true;
        }
        else {
            error = "Wrong value: " + m + " (must be \"inherit\", \"clear\" or \"user-default\")";

            return false;
        }
    }
};

template<typename O, typename C>
class callback_argument_parser_c : public string_argument_parser_c {
private:
    O o;
    C c;
    std::string v;
public:
    callback_argument_parser_c(O o, C c) : o(o), c(c), string_argument_parser_c(v) {}
    virtual bool set(const std::string &s) {
        (o->*c)(s);
        return true;
    }
};

template<typename O, typename C>
class function_argument_parser_c : public string_argument_parser_c {
private:
    O o;
    C c;
    std::string v;
public:
    function_argument_parser_c(O o, C c) : o(o), c(c), string_argument_parser_c(v) {}
    virtual bool set(const std::string &s) {
        if ((o->*c)(s)) {
            return true;
        }
        error = default_error;
        return false;
    }
};

typedef callback_argument_parser_c<options_class*, void(options_class::*)(const std::string&)> options_callback_argument_parser_c;

template<typename T, unit_t u, degrees_enum d>
class unit_argument_parser_c : public base_argument_parser_c < T > {
public:
    unit_argument_parser_c(T &value) : base_argument_parser_c<T>(value) {}
    virtual std::string value_description() { return "<value>[unit]"; }
    virtual bool set(const std::string &s) {
        try {
            this->value = convert(value_t(u, d), s, restriction_no_limit);
        }
        catch (std::string &str) {
            this->error = "Value type is not compatible: " + str;
            return false;
        }
        return true;
    }
};

template<degrees_enum d>
class time_argument_parser_c : public unit_argument_parser_c < restriction_t, unit_time_second, d > {
protected:
    virtual bool accept_zero() {
        return false;
    }
public:
    time_argument_parser_c(restriction_t &value) : unit_argument_parser_c<restriction_t, unit_time_second, d>(value) {}
    virtual bool set(const std::string &s) {
        if (!unit_argument_parser_c<restriction_t, unit_time_second, d>::set(s)) {
            return false;
        }
        if (this->value == 0 && !accept_zero()) {
            this->error = "Time cannot be set to 0";
            return false;
        }
        return true;
    }
};

typedef time_argument_parser_c<degree_milli> millisecond_argument_parser_c;
typedef time_argument_parser_c<degree_micro> microsecond_argument_parser_c;

class byte_argument_parser_c : public unit_argument_parser_c < restriction_t, unit_memory_byte, degree_default > {
protected:
    virtual bool accept_zero() {
        return false;
    }
public:
    byte_argument_parser_c(restriction_t &value) : unit_argument_parser_c<restriction_t, unit_memory_byte, degree_default>(value) {}
    virtual bool set(const std::string &s) {
        if (!unit_argument_parser_c<restriction_t, unit_memory_byte, degree_default>::set(s)) {
            return false;
        }
        if (value == 0 && !accept_zero()) {
            error = "Memory cannot be set to 0";
            return false;
        }
        return true;
    }
};

class percent_argument_parser_c : public unit_argument_parser_c < restriction_t, unit_no_unit, degree_m4 > {
protected:
    virtual bool accept_zero() {
        return false;
    }
public:
    percent_argument_parser_c(restriction_t &value) : unit_argument_parser_c<restriction_t, unit_no_unit, degree_m4>(value) {}
    virtual bool set(const std::string &s) {
        if (!unit_argument_parser_c<restriction_t, unit_no_unit, degree_m4>::set(s)) {
            return false;
        }
        if (value == 0 && !accept_zero()) {
            error = "Ratio cannot be set to 0";
            return false;
        }
        return true;
    }
};

template<typename T, bool inverted = false>
class base_boolean_argument_parser_c : public base_argument_parser_c < T > {
protected:
    virtual T true_value() {
        return !inverted;
    }
    virtual T false_value() {
        return inverted;
    }
public:
    base_boolean_argument_parser_c(T &value) : base_argument_parser_c<T>(value) {}
    virtual std::string value_description() { return "0|1"; }
    virtual bool set(const std::string &s) {
        if (s == "1") {
            this->value = true_value();
        }
        else if (s == "0") {
            this->value = false_value();
        }
        else {
            this->error = "Value must be either 0 or 1";
            return false;
        }
        return true;
    }
};

typedef base_boolean_argument_parser_c<bool> boolean_argument_parser_c;
typedef base_boolean_argument_parser_c<bool, true> inverted_boolean_argument_parser_c;

class bool_restriction_argument_parser_c : public base_boolean_argument_parser_c < restriction_t > {
public:
    bool_restriction_argument_parser_c(restriction_t &value) : base_boolean_argument_parser_c<restriction_t>(value) {}
protected:
    virtual restriction_t true_value() {
        return restriction_limited;
    }
    virtual restriction_t false_value() {
        return restriction_no_limit;
    }
};

class function_restriction_argument_parser_c : public abstract_argument_parser_c {
};
