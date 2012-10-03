#include <ParseArguments.h>
#include <process.h>
#include <pipes.h>
#include <iostream>
#include "report.h"
#include "uconvert.h"

// Formatting report
string format_report(CReport rep)
{
    std::ostringstream osstream;
    osstream << std::endl << "--------------- Spawner report ---------------" << std::endl;
    osstream << "Application:               " << rep.application_name << std::endl;
    osstream << "Parameters:                " << rep.options.get_arguments() << std::endl; 
    osstream << "SecurityLevel:             " << (rep.restrictions.get_restriction(restriction_security_limit) == restriction_limited) << std::endl;
    osstream << "CreateProcessMethod:       " << std::endl;
    osstream << "UserName:                  " << std::endl;
    osstream << "UserTimeLimit:             " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "DeadLine:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.restrictions.get_restriction(restriction_processor_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "MemoryLimit:               " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "WriteLimit:                " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "UserTime:                  " << 
        convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.processor_time/10.0, " (u)") << std::endl;
    osstream << "PeakMemoryUsed:            " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (du)") << std::endl;
    osstream << "Written:                   " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_kilo), rep.write_transfer_count, " (du)") << std::endl;
    osstream << "TerminateReason:           " << get_terminate_reason(rep.terminate_reason) << std::endl;
    osstream << "ExitStatus:                " << get_status_text(rep.process_status) << std::endl;
    osstream << "ExitCode:                  " << rep.exit_code << std::endl;
    osstream << "Exception:                 " << get_exception_name(rep.exception) << std::endl;
    osstream << "ExceptionInterpritation:   " << get_exception_text(rep.exception) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "SpawnerError:              " << std::endl;
    return osstream.str();
}

int main(int argc, char *argv[])
{
	CArguments arguments(argc, argv);
    if (!arguments.valid())
    {
        arguments.ShowUsage();
        return 0;
    }

    CRestrictions restrictions;
    COptions options;

    for (int i = arguments.get_arguments_index(); i < argc; i++)
        options.add_argument(argv[i]);

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

    CProcess process(arguments.get_program());
    process.set_restrictions(restrictions);
    process.set_options(options);

    process.Run();

    CReport rep = process.get_report();

    cout << format_report(rep);
	return 0;
}