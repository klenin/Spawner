#include <ParseArguments.h>
#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>

// Formatting report
std::string format_report(const report_class &rep, const options_class &options, const restrictions_class &restrictions)
{
    std::ostringstream osstream;
    osstream << std::endl << "--------------- Spawner report ---------------" << std::endl;
    osstream << "Application:               " << rep.application_name << std::endl;
    osstream << "Parameters:                " << options.get_arguments() << std::endl;
    osstream << "Working directory:         " << rep.working_directory << std::endl;
    osstream << "Silent:                    " << options.silent_errors << std::endl;
    osstream << "Debug:                     " << options.debug << std::endl;
    osstream << "HideUI:                    " << options.hide_gui << std::endl;
    osstream << "SecurityLevel:             " << (restrictions.get_restriction(restriction_security_limit) == restriction_limited) << std::endl;
    osstream << "CreateProcessMethod:       " << (options.login==""?"Default":"WithLogon") << std::endl;
    osstream << "UserName:                  " << rep.login << std::endl;
    osstream << "UserTimeLimit:             " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), restrictions.get_restriction(restriction_processor_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "Deadline:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "MemoryLimit:               " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "WriteLimit:                " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "LoadRatioLimit:            " << convert(value_t(unit_no_unit, degree_m4), value_t(unit_no_unit), restrictions.get_restriction(restriction_load_ratio), " (%)", restriction_no_limit) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "ProcessorTime:             " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.processor_time/10.0, " (u)") << std::endl;
    osstream << "KernelTime:                " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.kernel_time/10.0, " (u)") << std::endl;
    osstream << "UserTime:                  " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), rep.user_time, " (u)") << std::endl;
    osstream << "PeakMemoryUsed:            " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (du)") << std::endl;
    osstream << "Written:                   " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.write_transfer_count, " (du)") << std::endl;
    osstream << "LoadRatio:                 " << convert(value_t(unit_no_unit, degree_centi), value_t(unit_no_unit), rep.load_ratio, " (%)", restriction_no_limit) << std::endl;
    osstream << "TerminateReason:           " << get_terminate_reason(rep.terminate_reason) << std::endl;
    osstream << "ExitStatusString:          " << get_status_text(rep.process_status) << std::endl;
    osstream << "ExitStatus:                " << rep.exit_code << std::endl;
    osstream << "Exception:                 " << get_exception_name(rep.exception) << std::endl;
    osstream << "ExceptionInterpretation:   " << get_exception_text(rep.exception) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "SpawnerError:              " << error_list::pop_error() << std::endl;
    return osstream.str();
}

int main(int argc, char *argv[])
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	CArguments arguments(argc, argv);
    if (!arguments.valid())
    {
        arguments.ShowUsage();
        return 0;
    }

    restrictions_class restrictions;
    options_class options(session_class::base_session);

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

    if (arguments.ArgumentExists(SP_SECURITY_LEVEL) && arguments.GetArgument(SP_SECURITY_LEVEL) == "1")
    {
        restrictions.set_restriction(restriction_security_limit, restriction_limited);
    }

    if (arguments.ArgumentExists(SP_WORKING_DIRECTORY))
        options.working_directory = arguments.GetArgument(SP_WORKING_DIRECTORY);

    if (arguments.ArgumentExists(SP_SILENT)) {
        options.silent_errors = true;
    }
    if (arguments.ArgumentExists(SP_LOGIN)) {
        options.login = arguments.GetArgument(SP_LOGIN);
    }
    if (arguments.ArgumentExists(SP_PASSWORD)) {
        options.password = arguments.GetArgument(SP_PASSWORD);
    }
    if (arguments.ArgumentExists(SP_HIDE_GUI)) {
        options.hide_gui = true;
    }
    if (arguments.ArgumentExists(SP_CMD)) {
	    options.use_cmd = true;
    }
    if (arguments.ArgumentExists(SP_DELEGATED_SESSION)) {
        options.session_id = arguments.GetArgument(SP_DELEGATED_SESSION);
    } 
    secure_runner *secure_runner_instance;
    if (arguments.ArgumentExists(SP_DELEGATED)) {
        options.delegated = true;
        secure_runner_instance = new delegate_runner(arguments.get_program(), options, restrictions);
    } else if (arguments.ArgumentExists(SP_DELEGATED_SESSION)){
        secure_runner_instance = new delegate_instance_runner(arguments.get_program(), options, restrictions);
    } else {
        secure_runner_instance = new secure_runner(arguments.get_program(), options, restrictions);
    }

    if (!arguments.ArgumentExists(SP_DELEGATED_SESSION)) {
        if (arguments.ArgumentExists(SP_OUTPUT_FILE)) {
			std::vector<output_buffer_class *> output_buffers;
			for (int i = 0; i < arguments.ArgumentCount(SP_OUTPUT_FILE); ++i) {
				output_buffer_class *output_buffer = NULL;
				if (arguments.GetArgument(SP_OUTPUT_FILE, i) == "std") {
					output_buffer = new output_stdout_buffer_class(4096);
				} else {
					output_buffer = new output_file_buffer_class(arguments.GetArgument(SP_OUTPUT_FILE, i), 4096);
				}
				output_buffers.push_back(output_buffer);
			}
            secure_runner_instance->set_pipe(STD_OUTPUT_PIPE, new output_pipe_class(session_class::base_session, "stdout", output_buffers));
        }

        if (arguments.ArgumentExists(SP_ERROR_FILE)) {
			std::vector<output_buffer_class *> output_buffers;
			for (int i = 0; i < arguments.ArgumentCount(SP_ERROR_FILE); ++i) {
				output_buffer_class *output_buffer = NULL;
				if (arguments.GetArgument(SP_ERROR_FILE, i) == "std") {
					output_buffer = new output_stdout_buffer_class(4096);
				} else {
					output_buffer = new output_file_buffer_class(arguments.GetArgument(SP_ERROR_FILE, i), 4096);
				}
				output_buffers.push_back(output_buffer);
			}
            secure_runner_instance->set_pipe(STD_ERROR_PIPE, new output_pipe_class(session_class::base_session, "stderr", output_buffers));
        }

        if (arguments.ArgumentExists(SP_INPUT_FILE)) {
            input_buffer_class *input_buffer = NULL;
            if (arguments.GetArgument(SP_INPUT_FILE) == "std") {
                input_buffer = new input_buffer_class(4096);
            } else {
                input_buffer = new input_file_buffer_class(arguments.GetArgument(SP_INPUT_FILE), 4096);
            }
            secure_runner_instance->set_pipe(STD_INPUT_PIPE, new input_pipe_class(session_class::base_session, "stdin", input_buffer));
        }
    }

    secure_runner_instance->run_process();
    report_class rep = secure_runner_instance->get_report();
    if (!arguments.ArgumentExists(SP_HIDE_REPORT))
        std::cout << format_report(rep, options, restrictions);
    if (arguments.ArgumentExists(SP_REPORT_FILE))
    {
        std::ofstream fo(arguments.GetArgument(SP_REPORT_FILE).c_str());
        fo << format_report(rep, options, restrictions);
        fo.close();
    }
    delete secure_runner_instance;
	return 0;
}
