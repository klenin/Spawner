#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>
#include "report.h"

int main(int argc, char *argv[])
{
	CArguments arg(argc, argv);
    CAsyncProcess process("C:\\GnuWin32\\bin\\ls.exe");//J:\\Projects\\charity\\test.exe");
    //process.SetRestrictionForKind(RESTRICTION_MEMORY_LIMIT, 2000000);
    //process.SetRestrictionForKind(RESTRICTION_USER_TIME_LIMIT, 2000);
    //process.SetRestrictionForKind(RESTRICTION_SECURITY_LIMIT, RESTRICTION_LIMITED);
    process.Run();
    while (process.is_running())
    {
        Sleep(1);
    }
    if (process.get_process_status() == process_finished_abnormally)
        cout << endl << get_exception_info(process.get_exception(), "%n: %t\n");
    process.WaitAndFinish();
    string s;
    getline(process.stderror(), s);
    cout << "!!!!" << s;
	return 0;
}