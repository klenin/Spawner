#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>
#include "report.h"
#include "uconvert.h"

/*struct restricts
{
    spawner_arguments argument;
    restriction_t type;
    unit_t unit;
    degrees_enum degree;
};

const restricts argument_restriction[] = {
    {SP_MEMORY_LIMIT,   restriction_memory_limit, unit_memory_byte, degree_default},
    {SP_WRITE_LIMIT,    restriction_write_limit, unit_memory_byte, degree_default},
    {SP_DEADLINE,       restriction_processor_time_limit, unit_time_second, degree_milli},
    {SP_TIME_LIMIT,   restriction_memory_limit, unit_memory_byte, degree_default},
    {SP_MEMORY_LIMIT,   restriction_memory_limit, unit_memory_byte, degree_default},
    {SP_MEMORY_LIMIT,   restriction_memory_limit, unit_memory_byte, degree_default},
    {SP_MEMORY_LIMIT,   restriction_memory_limit, unit_memory_byte, degree_default},
};*/

int main(int argc, char *argv[])
{
	CArguments arguments(argc, argv);
    if (!arguments.valid())
        return 0;

    CRestrictions restrictions;

    //make this automatic

    if (arguments.ArgumentExists(SP_MEMORY_LIMIT))
    {
        restrictions.set_restriction(restriction_memory_limit, convert(value_t(unit_memory_byte), 
            arguments.GetArgument(SP_MEMORY_LIMIT), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_WRITE_LIMIT))
    {
        restrictions.set_restriction(restriction_write_limit, convert(value_t(unit_memory_byte), 
            arguments.GetArgument(SP_WRITE_LIMIT), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_DEADLINE))
    {
        restrictions.set_restriction(restriction_processor_time_limit, convert(value_t(unit_time_second, degree_milli), 
            arguments.GetArgument(SP_DEADLINE), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_TIME_LIMIT))
    {
        restrictions.set_restriction(restriction_user_time_limit, convert(value_t(unit_time_second, degree_milli), 
            arguments.GetArgument(SP_TIME_LIMIT), restriction_no_limit));
    }

    CProcess process("J:\\Projects\\charity\\test\\memory\\memory.exe");//"J:\\Projects\\study\\fortune\\Release\\voronoy.exe");//J:\\Projects\\charity\\test.exe");//"C:\\GnuWin32\\bin\\ls.exe");
    process.set_restrictions(restrictions);

    process.Run();
    CReport rep = process.get_report();

    string s;
    getline(process.stderror(), s);
    cout << "!!!!" << s;
    double sec = convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second, degree_default), rep.processor_time/10.0);
    double memory = convert(value_t(unit_memory_byte, degree_default), value_t(unit_memory_byte, degree_mega), (double)rep.peak_memory_used);
    double written = convert(value_t(unit_memory_byte, degree_default), value_t(unit_memory_byte, degree_mega), (double)rep.write_transfer_count);
	return 0;
}