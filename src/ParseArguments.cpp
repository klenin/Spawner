#include "ParseArguments.h"
#include <SimpleOpt.h>
#include <SimpleGlob.h>
#include <stdio.h>
#include <iostream>

/*
  -ml:[n]            SP_MEMORY_LIMIT      Максимальный объем виртуальной памяти, выделенный процессу (в Mb).
  -tl:[n]            SP_TIME_LIMIT        Максимальное время выполнения процесса в пользовательском режиме (в сек).
  -d:[n]             SP_DEADLINE          Лимит физического времени, выделенного процессу (в сек).
  -wl:[n]            SP_WRITE_LIMIT       Максимальный объем данных, который может быть записан процессом (в Mb).
  -u:[user@domain]   SP_USER              �?мя пользователя в формате: User[@Domain]
  -p:[password]      SP_PASSWORD          Пароль.
  -runas:[0|1]       SP_RUNAS             �?спользовать сервис RunAs для запуска процесса.
  -s:[n]             SP_SECURITY_LEVEL    Уровень безопасности. Может принимать значения 0 или 1.
  -hr:[0|1]          SP_HIDE_REPORT       Не показывать отчет.
  -ho:[0|1]          SP_HIDE_OUTPUT       Не показывать выходной поток (STDOUT) приложения. -->Show Output
  -sr:[file]         SP_REPORT_FILE       Сохранить отчет в файл. --> rf better, i think
  -so:[file]         SP_OUTPUT_FILE       Сохранить выходной поток в файл.
  -i:[file]          SP_INPUT_FILE        Получить входной поток из файла.
  // write stdout to stdout
  //       stderr to stdout
  // disable gui
  // disable errors(send & not send window)
  //
*/
// <-- move this to some sort of header
// check if long attributes can be applied 
CSimpleOpt::SOption Options[] = 
{   
    {SP_HELP,               "--help",	SO_NONE},
    {SP_MEMORY_LIMIT,       "-ml",      SO_REQ_CMB},
    {SP_TIME_LIMIT,         "-tl",      SO_REQ_CMB},
    {SP_DEADLINE,           "-d" ,      SO_REQ_CMB},
    {SP_WRITE_LIMIT,        "-wl",      SO_REQ_CMB},
    {SP_LOGIN,              "-u" ,      SO_REQ_CMB},
    {SP_DELEGATED,          "--delegated",SO_NONE},
    {SP_DELEGATED_SESSION,  "--session",SO_REQ_CMB},
    {SP_DEBUG,              "--debug",  SO_NONE},
    {SP_PASSWORD,	        "-p" ,      SO_REQ_CMB},
    {SP_LOAD_RATIO,	        "-lr" ,     SO_REQ_CMB},
//	{SP_RUNAS,
    {SP_SECURITY_LEVEL,     "-s" ,      SO_REQ_CMB},
    {SP_HIDE_REPORT,        "-hr",      SO_NONE},
    {SP_HIDE_GUI,           "-hg" ,     SO_NONE},
    {SP_HIDE_GUI,           "-sw" ,     SO_REQ_CMB},
    {SP_SILENT,             "--silent", SO_NONE},
    {SP_CMD,                "--cmd",    SO_NONE},
    {SP_CMD,                "--systempath",SO_NONE},
    {SP_REPORT_FILE,        "-sr",      SO_REQ_CMB},
    {SP_OUTPUT_FILE,        "--out",    SO_REQ_CMB},
    {SP_INPUT_FILE,         "--in" ,    SO_REQ_CMB},
    {SP_OUTPUT_FILE,        "-so",      SO_REQ_CMB},
    {SP_INPUT_FILE,         "-i" ,      SO_REQ_CMB},
    {SP_ERROR_FILE,         "--err" ,   SO_REQ_CMB},
    {SP_WORKING_DIRECTORY,  "-wd" ,     SO_REQ_CMB},

    {SP_DELEGATED,          "-runas",   SO_REQ_CMB},
    {SP_SHOW_OUTPUT,        "-ho",      SO_REQ_CMB},
    {SP_ERROR_FILE,         "-se",      SO_NONE},
    SO_END_OF_OPTIONS
};

// Extracting program to execute and it's arguments
// Getting spawner options
CArguments::CArguments(int argc, char *argv[])
{
    int i = 1;
    for (i = 1 ; i < argc; i++)
        if (*argv[i] != '-' && *argv[i] != '/') // please do not run programs which name starts with '-' or '/'
            break;
    v = false;

    if (i >= argc)
        return;

	CSimpleOpt args(i, argv, Options);

    while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
            if (args.OptionId() == SP_HELP) {
                ShowUsage();
            }
            /*
            printf("Option, ID: %d, Text: '%s', Argument: '%s'\n",
                args.OptionId(), args.OptionText(),
                args.OptionArg() ? args.OptionArg() : "");
                //*/
			//spawner_arhuments sp_arguments = (spawner_arguments)args.OptionId();
			//if (arguments.find(sp_arguments) == arguments.end()) 
            arguments[(spawner_arguments)args.OptionId()].push_back(args.OptionArg() ? args.OptionArg() : "");// = args.OptionArg() ? args.OptionArg() : "";
        }
        else {
            printf("Invalid argument: %s\nUse spawner --help for details\n", args.OptionText());
            return;
        }
    }
    v = true;

    // retrieving program, it's name and arguments

    program = argv[i];
    arguments_index = i+1; // not always
}

CArguments::~CArguments()
{

}

bool CArguments::valid()
{
    return v;
}

int CArguments::get_arguments_index()
{
    return arguments_index;
}

std::string CArguments::get_program()
{
    return program;
}

void CArguments::ShowUsage()
{
    //writing usage
    std::cout << "Usage: \tspawner [options] program [program options]" << std::endl;
    std::cout << "\tspawner --help   to get this message" << std::endl;
}

std::string CArguments::GetArgument(const spawner_arguments &key, const int &index)
{
	if (arguments.find(key) == arguments.end()) {
		return "";
	}
    return arguments[key][index];
}

bool CArguments::ArgumentExists(const spawner_arguments &key)
{
    return arguments.find(key) != arguments.end();
}

size_t CArguments::ArgumentCount(const spawner_arguments &key) {
	if (arguments.find(key) == arguments.end()) {
		return 0;
	}
	return arguments[key].size();
}
