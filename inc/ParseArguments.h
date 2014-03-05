#ifndef _PARSE_ARGUMENTS_H_
#define _PARSE_ARGUMENTS_H_

#include <map>
#include <string>
#include <vector>

enum spawner_arguments {
    SP_HELP,
    SP_APPLICATION,
    SP_MEMORY_LIMIT,
    SP_TIME_LIMIT,
    SP_DEADLINE,
    SP_WRITE_LIMIT,
    SP_LOAD_RATIO,
    SP_IDLE_TIME_LIMIT,
    SP_LOGIN,
    SP_PASSWORD,
    SP_RUNAS,
    SP_SECURITY_LEVEL,
    SP_HIDE_REPORT,
    SP_HIDE_OUTPUT,
    SP_SHOW_STDERR,
    SP_REPORT_FILE,
    SP_OUTPUT_FILE,
    SP_INPUT_FILE,
    SP_ERROR_FILE,
    SP_WORKING_DIRECTORY,
    SP_SILENT,
    SP_DEBUG,
    SP_HIDE_GUI,
    SP_CMD,
    SP_DELEGATED,
    SP_DELEGATED_SESSION,
    SP_SEPARATOR,
    SP_PROGRAM_ID,
    SP_JSON,
};

enum arguments_state_e {
    arguments_state_ok,
    arguments_state_help,
    arguments_state_error,
};


class argument_set_c {
private:
    std::map<spawner_arguments, std::vector<std::string> > arguments;
    std::string program;
    std::vector<std::string> program_arguments;
public:
    bool argument_exists(const spawner_arguments &key) const;
    void add_argument(const spawner_arguments &key, const std::string &value = "");
    void add_program_argument(const std::string &argument);
    void set_program(const std::string &program_arg);
    std::string get_argument(const spawner_arguments &key, const unsigned &index = 0) const;
    std::string get_program() const;
    const std::vector<std::string> &get_program_arguments() const;
    size_t get_argument_count(const spawner_arguments &key) const;
};



class arguments_c
{
private:
    bool v;
    std::string program;
    int arguments_index;
    std::vector<std::string> arguments;
    std::vector<argument_set_c> argument_sets;
    arguments_state_e state;
    void init_program(argument_set_c &argument_set, const int &start, const int &end);
public:
	arguments_c(int argc, char *argv[]);
	~arguments_c();
    bool valid();
    void ShowUsage();
    int get_arguments_index() const;
    std::string get_program() const;
    arguments_state_e get_state() const;
    const argument_set_c &get_argument_set(const int &index = 0) const;
	size_t get_argument_set_count() const;
};

#endif//_PARSE_ARGUMENTS_H_
