#include <ParseArguments.h>
#include <iostream>

#include <spawner.h>

// Formatting report
string format_report(report_class rep)
{
    std::ostringstream osstream;
    osstream << std::endl << "--------------- Spawner report ---------------" << std::endl;
    osstream << "Application:               " << rep.application_name << std::endl;
    osstream << "Working directory:         " << rep.options.working_directory << std::endl;
    osstream << "Parameters:                " << rep.options.get_arguments() << std::endl; 
    osstream << "SecurityLevel:             " << (rep.restrictions.get_restriction(restriction_security_limit) == restriction_limited) << std::endl;
    osstream << "CreateProcessMethod:       " << (rep.options.login==""?"Default":"WithLogon") << std::endl;
    osstream << "UserName:                  " << rep.options.login << std::endl;
    osstream << "UserTimeLimit:             " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "DeadLine:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.restrictions.get_restriction(restriction_processor_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "MemoryLimit:               " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "WriteLimit:                " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "LoadRatioLimit:            " << convert(value_t(unit_no_unit, degree_m4), value_t(unit_no_unit), rep.restrictions.get_restriction(restriction_load_ratio), " (%)", restriction_no_limit) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "ProcessorTime:             " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.processor_time/10.0, " (u)") << std::endl;
    osstream << "KernelTime:                " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.kernel_time/10.0, " (u)") << std::endl;
    osstream << "UserTime:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.user_time, " (u)") << std::endl;
    osstream << "PeakMemoryUsed:            " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (du)") << std::endl;
    osstream << "Written:                   " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_kilo), rep.write_transfer_count, " (du)") << std::endl;
    osstream << "LoadRatio:                 " << convert(value_t(unit_no_unit, degree_centi), value_t(unit_no_unit), rep.load_ratio, " (%)", restriction_no_limit) << std::endl;
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

    restrictions_class restrictions;
    options_class options;

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

    if (arguments.ArgumentExists(SP_TIME_LIMIT))
    {
        restrictions.set_restriction(restriction_processor_time_limit, convert(value_t(unit_time_second, degree_milli), 
            arguments.GetArgument(SP_TIME_LIMIT), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_DEADLINE))
    {
        restrictions.set_restriction(restriction_user_time_limit, convert(value_t(unit_time_second, degree_milli), 
            arguments.GetArgument(SP_DEADLINE), restriction_no_limit));
    }

    if (arguments.ArgumentExists(SP_LOAD_RATIO))
    {
        restrictions.set_restriction(restriction_load_ratio, convert(value_t(unit_no_unit, degree_m4), 
            arguments.GetArgument(SP_LOAD_RATIO), restriction_no_limit));//dirty hack
    }

    if (arguments.ArgumentExists(SP_WORKING_DIRECTORY))
        options.working_directory = arguments.GetArgument(SP_WORKING_DIRECTORY);

    if (arguments.ArgumentExists(SP_SILENT))
        options.silent_errors = true;
    if (arguments.ArgumentExists(SP_LOGIN))
        options.silent_errors = true;
    options.hide_gui = true;

    process_wrapper wrapper(arguments.get_program(), options, restrictions);
    wrapper.run_process();
    report_class rep = wrapper.get_report();

    cout << format_report(rep);
	return 0;
}