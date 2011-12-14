#include <ParseArguments.h>
#include <SimpleOpt.h>
#include <SimpleGlob.h>
#include <stdio.h>

enum
{
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
//	SP_SHOW_STDERR,
	SP_REPORT_FILE,
	SP_OUTPUT_FILE,
	SP_INPUT_FILE,
};

/*
  -ml:[n]            SP_MEMORY_LIMIT      Максимальный объем виртуальной памяти, выделенный процессу (в Mb).
  -tl:[n]            SP_TIME_LIMIT        Максимальное время выполнения процесса в пользовательском режиме (в сек).
  -d:[n]             SP_DEADLINE          Лимит физического времени, выделенного процессу (в сек).
  -wl:[n]            SP_WRITE_LIMIT       Максимальный объем данных, который может быть записан процессом (в Mb).
  -u:[user@domain]   SP_USER              Имя пользователя в формате: User[@Domain]
  -p:[password]      SP_PASSWORD          Пароль.
  -runas:[0|1]       SP_RUNAS             Использовать сервис RunAs для запуска процесса.
  -s:[n]             SP_SECURITY_LEVEL    Уровень безопасности. Может принимать значения 0 или 1.
  -hr:[0|1]          SP_HIDE_REPORT       Не показывать отчет.
  -ho:[0|1]          SP_HIDE_OUTPUT       Не показывать выходной поток (STDOUT) приложения. -->Show Output
  -sr:[file]         SP_REPORT_FILE       Сохранить отчет в файл. --> rf better, i think
  -so:[file]         SP_OUTPUT_FILE       Сохранить выходной поток в файл.
  -i:[file]          SP_INPUT_FILE        Получить входной поток из файла.
*/
// <-- move this to some sort of header
// check if long attributes can be applied 
CSimpleOpt::SOption Options[] = 
{
	{SP_MEMORY_LIMIT, 	"-ml",	SO_REQ_CMB},
	{SP_TIME_LIMIT, 	"-tl",	SO_REQ_CMB},
	{SP_DEADLINE,		"-d" ,	SO_REQ_CMB},
	{SP_WRITE_LIMIT,	"-wl",	SO_REQ_CMB},
	{SP_USER,		"-u" ,	SO_REQ_CMB},
	{SP_PASSWORD,		"-p" ,	SO_REQ_CMB},
//	{SP_RUNAS,
	{SP_SECURITY_LEVEL,	"-s" ,	SO_NONE},
	{SP_HIDE_REPORT,	"-hr",	SO_NONE},
	{SP_SHOW_OUTPUT,	"-so",	SO_NONE},
//	{SP_SHOW_STDERR,	"-se",	SO_NONE},
	{SP_REPORT_FILE,	"-sr",	SO_REQ_CMB},
	{SP_OUTPUT_FILE,	"-so",	SO_REQ_CMB},
	{SP_INPUT_FILE,		"-i" ,	SO_REQ_CMB},
	SO_END_OF_OPTIONS
};

CArguments::CArguments(int argc, char *argv[])
{
	CSimpleOpt args(argc, argv, Options);
    while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
//            if (args.OptionId() == OPT_HELP) {
//                ShowUsage();
//                return 0;
//            }
            printf("Option, ID: %d, Text: '%s', Argument: '%s'\n",
                args.OptionId(), args.OptionText(),
                args.OptionArg() ? args.OptionArg() : "");
        }
        else {
            printf("Invalid argument: %s\n", args.OptionText());
        }
    }
}

CArguments::~CArguments()
{

}
