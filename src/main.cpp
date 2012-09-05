#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>
#include "report.h"

int main(int argc, char *argv[])
{
	CArguments arg(argc, argv);
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    CAsyncProcess process("J:\\Projects\\charity\\test\\multi_threaded\\build\\Debug\\multi_threaded.exe");//"J:\\Projects\\study\\fortune\\Release\\voronoy.exe");//J:\\Projects\\charity\\test.exe");//"C:\\GnuWin32\\bin\\ls.exe");
    //process.SetRestrictionForKind(RESTRICTION_MEMORY_LIMIT, 2000000);
    process.SetRestrictionForKind(restriction_processor_time_limit, 5000);
    //process.SetRestrictionForKind(RESTRICTION_SECURITY_LIMIT, RESTRICTION_LIMITED);
    process.Run();
    int i = 0;
    while (process.is_running())
    {
        i++;
        if (i == 1000)
        {
            process.suspend();
            Sleep(1);
        }
        if (i == 5000)
        {
            process.resume();
            Sleep(1);
        }
        Sleep(1);
    }
    if (process.get_process_status() == process_finished_abnormally)
        cout << endl << get_exception_info(process.get_exception(), "%n: %t\n");
    process.WaitAndFinish();// move this to corresponding thread
    string s;
    getline(process.stderror(), s);
    cout << "!!!!" << s;
	return 0;
}