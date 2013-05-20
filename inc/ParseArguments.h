#ifndef _PARSE_ARGUMENTS_H_
#define _PARSE_ARGUMENTS_H_

#include <map>
#include <string>
#include <vector>

typedef enum
{
    SP_HELP,
    SP_APPLICATION,
    SP_MEMORY_LIMIT,
    SP_TIME_LIMIT,
    SP_DEADLINE,
    SP_WRITE_LIMIT,
    SP_LOGIN,
    SP_PASSWORD,
    SP_RUNAS,
    SP_SECURITY_LEVEL,
    SP_HIDE_REPORT,
    SP_SHOW_OUTPUT,
    SP_SHOW_STDERR,
    SP_REPORT_FILE,
    SP_OUTPUT_FILE,
    SP_INPUT_FILE,
    SP_ERROR_FILE,
    SP_LOAD_RATIO,
    SP_WORKING_DIRECTORY,
    SP_SILENT,
    SP_DEBUG,
    SP_HIDE_GUI,
    SP_CMD,
    SP_DELEGATED,
    SP_DELEGATED_SESSION,
} spawner_arguments;


class CArguments
{
private:
    bool v;
    std::string program;
    int arguments_index;
    std::map<spawner_arguments, std::vector<std::string> > arguments;
public:
	CArguments(int argc, char *argv[]);
	~CArguments();
    bool valid();
    void ShowUsage();
    int get_arguments_index();
    std::string get_program();
    std::string GetArgument(const spawner_arguments &key, const int &index = 0);
	size_t ArgumentCount(const spawner_arguments &key);
    bool ArgumentExists(const spawner_arguments &key);
};

#endif//_PARSE_ARGUMENTS_H_
