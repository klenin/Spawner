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
    //old options
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
    //new options
    {SP_LOAD_RATIO,	        "-lr" ,     SO_REQ_CMB},
    {SP_IDLE_TIME_LIMIT,    "--idle" ,     SO_REQ_CMB},
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
    {SP_HIDE_OUTPUT,        "-ho",      SO_REQ_CMB},
    {SP_ERROR_FILE,         "-se",      SO_NONE},
    {SP_SEPARATOR,          "--separator", SO_REQ_CMB},
    {SP_PROGRAM_ID,         "--program", SO_REQ_CMB},
    {SP_JSON,               "--json",   SO_NONE},
    SO_END_OF_OPTIONS
};


bool argument_set_c::argument_exists(const spawner_arguments &key) const {
    return arguments.find(key) != arguments.end();
} 
void argument_set_c::add_argument(const spawner_arguments &key, const std::string &value){
    arguments[key].push_back(value);
}
void argument_set_c::add_program_argument(const std::string &argument) {
    program_arguments.push_back(argument);
}
void argument_set_c::set_program(const std::string &program_arg) {
    program = program_arg;
}
std::string argument_set_c::get_argument(const spawner_arguments &key, const unsigned &index) const {
    if (!argument_exists(key) || index >= arguments.at(key).size()) {
        return "";
    }
    return arguments.at(key)[index];
}
size_t argument_set_c::get_argument_count(const spawner_arguments &key) const {
    if (!argument_exists(key)) {
        return 0;
    }
    return arguments.at(key).size();
}
std::string argument_set_c::get_program() const {
    return program;
}

const std::vector<std::string> &argument_set_c::get_program_arguments() const {
    return program_arguments;
}


void arguments_c::init_program(argument_set_c &argument_set, const int &start, const int &end) {
    argument_set.set_program(arguments[start]);
    for (int i = start + 1; i < end; ++i) {
        argument_set.add_program_argument(arguments[i]);
    }
}


// Extracting program to execute and it's arguments
// Getting spawner options
arguments_c::arguments_c(int argc, char *argv[]) {
	CSimpleOpt args(argc, argv, Options);
    if (argc <= 1) {
        state = arguments_state_error;
        return;
    }
    for (int i = 0; i < argc; ++i) {
        arguments.push_back(argv[i]);
    }

    std::string separator;
    state = arguments_state_ok;
    argument_sets.push_back(argument_set_c());
    std::vector<argument_set_c>::iterator argument_set = argument_sets.begin();

    while (args.Next()) {
        if (separator == args.OptionText()) {
            init_program(*argument_set, args.LastErrorPos() + 1, args.CurrentPos());
            args.ResetErrorPos();
            argument_sets.push_back(argument_set_c());
            argument_set = argument_sets.end();
            argument_set--;
            continue;
        }
        if (args.LastError() == SO_SUCCESS && args.CurrentPos() < args.LastErrorPos()) {
            if (args.OptionId() == SP_HELP) {
                state = arguments_state_help;
                return;
            }
            if (args.OptionId() == SP_SEPARATOR) {
                separator = std::string("--") + args.OptionArg();
            } else {
                /*
                printf("Option, ID: %d, Text: '%s', Argument: '%s'\n",
                    args.OptionId(), args.OptionText(),
                    args.OptionArg() ? args.OptionArg() : "");
                    //*/
                argument_set->add_argument((spawner_arguments)args.OptionId(), args.OptionArg() ? args.OptionArg() : "");
            }
        } else {
            //
            //program_detected = true;
            //argument_set->set_program(args.OptionText());
        }
        /*else {
            printf("Invalid argument: %s\nUse spawner --help for details\n", args.OptionText());
            return;
        }*/
    }
    if (args.LastErrorPos() == (unsigned)-1) {
        state = arguments_state_error;
        return;
    }

    init_program(*argument_set, args.LastErrorPos() + 1, args.CurrentPos() + 1);

    //v = true;

    // retrieving program, it's name and arguments

    //program = argv[i];
    //arguments_index = i+1; // not always
    return;
}

arguments_c::~arguments_c()
{

}

bool arguments_c::valid()
{
    return v;
}

int arguments_c::get_arguments_index() const {
    return arguments_index;
}

std::string arguments_c::get_program() const {
    return program;
}

void arguments_c::ShowUsage() {
    //writing usage
    std::cout << "Usage: \tspawner [options] program [program options]" << std::endl;
    std::cout << "\tspawner --help   to get this message" << std::endl;
}

const argument_set_c &arguments_c::get_argument_set(const int &index) const {
    return argument_sets[index];
}

arguments_state_e arguments_c::get_state() const {
    return state;
}

size_t arguments_c::get_argument_set_count() const {
	return argument_sets.size();
}
