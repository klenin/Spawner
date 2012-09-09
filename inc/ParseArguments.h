#ifndef _PARSE_ARGUMENTS_H_
#define _PARSE_ARGUMENTS_H_

#include <map>

using namespace std;

typedef enum
{
    SP_HELP,
    SP_APPLICATION,
    SP_MEMORY_LIMIT,
    SP_TIME_LIMIT,
    SP_DEADLINE,
    SP_WRITE_LIMIT,
    SP_USER,
    SP_PASSWORD,
    SP_RUNAS,
    SP_SECURITY_LEVEL,
    SP_HIDE_REPORT,
    SP_SHOW_OUTPUT,
    SP_SHOW_STDERR,
    SP_REPORT_FILE,
    SP_OUTPUT_FILE,
    SP_INPUT_FILE,
    SP_WORKING_DIRECTORY,
} spawner_arguments;


class CArguments
{
private:
    bool v;
public:
	CArguments(int argc, char *argv[]);
	~CArguments();
    bool valid();
    void ShowUsage();
    string GetArgument(const spawner_arguments &key);
    bool ArgumentExists(const spawner_arguments &key);
    map<spawner_arguments, string> arguments;
};

#endif//_PARSE_ARGUMENTS_H_
