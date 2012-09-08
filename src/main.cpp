#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>
#include "report.h"
#include "uconvert.h"

int main(int argc, char *argv[])
{
    convert(value_t(unit_memory_bit, degree_kilo), value_t(unit_memory_byte, degree_mega), 1500.0);
	CArguments arg(argc, argv);
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    CRestrictions restrictions;
    restrictions.set_restriction(restriction_memory_limit, 1024*1024*20);
    restrictions.set_restriction(restriction_processor_time_limit, 5000);
    CAsyncProcess process("J:\\Projects\\charity\\test\\memory\\memory.exe");//"J:\\Projects\\study\\fortune\\Release\\voronoy.exe");//J:\\Projects\\charity\\test.exe");//"C:\\GnuWin32\\bin\\ls.exe");
    process.set_restrictions(restrictions);

    process.RunAsync();
    int i = 0;
    while (process.is_running());
    CReport rep = process.get_report();
    if (rep.process_status == process_finished_abnormally)
        cout << endl << get_exception_info(rep.exception, "%n: %t\n");
    process.Finish();


    string s;
    getline(process.stderror(), s);
    cout << "!!!!" << s;
    double sec = convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second, degree_default), rep.processor_time/10.0);
    double memory = convert(value_t(unit_memory_byte, degree_default), value_t(unit_memory_byte, degree_mega), (double)rep.peak_memory_used);
    double written = convert(value_t(unit_memory_byte, degree_default), value_t(unit_memory_byte, degree_mega), (double)rep.write_transfer_count);
	return 0;
}