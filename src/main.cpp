#include <ParseArguments.h>
#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>
#include <inc/compatibility.h>

void init_options_from_arguments(options_class &options, CArguments &arguments) {
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
    if (arguments.ArgumentExists(SP_DELEGATED)) {
        options.delegated = true;
    } 
    if (arguments.ArgumentExists(SP_DELEGATED_SESSION)) {
        options.session_id = arguments.GetArgument(SP_DELEGATED_SESSION);
    } 
    if (arguments.ArgumentExists(SP_REPORT_FILE)) {
        options.report_file = arguments.GetArgument(SP_REPORT_FILE);
    }
    if (arguments.ArgumentExists(SP_HIDE_REPORT)) {
        options.hide_report = true;
    }
    if (arguments.ArgumentExists(SP_DEBUG)) {
        options.debug = true;
    }
    options.hide_gui = true;
}

output_buffer_class *create_output_buffer(const std::string &name, const pipes_t &pipe_type, const size_t buffer_size = 4096) {
	output_buffer_class *output_buffer = NULL;
	if (name == "std") {
        unsigned int color = FOREGROUND_BLUE | FOREGROUND_GREEN;
        if (pipe_type == STD_ERROR_PIPE) {
            color = FOREGROUND_RED;
        }
		output_buffer = new output_stdout_buffer_class(4096, color);
	} else {
		output_buffer = new output_file_buffer_class(name, 4096);
	}
    return output_buffer;
}

input_buffer_class *create_input_buffer(const std::string &name, const size_t buffer_size = 4096) {
    input_buffer_class *input_buffer = NULL;
    if (name == "std") {
        input_buffer = new input_buffer_class(4096);
    } else {
        input_buffer = new input_file_buffer_class(name, 4096);
    }
    return input_buffer;
}



int main(int argc, char *argv[])
{
	//SetConsoleCP(1251);
	//SetConsoleOutputCP(1251);

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

    ReadEnvironmentVariables(options, restrictions);

    //make this automatic
    const struct {
        spawner_arguments argument;
        restriction_kind_t restriction;
    } restriction_bindings[] = {
        {SP_TIME_LIMIT, restriction_processor_time_limit},
        {SP_MEMORY_LIMIT, restriction_memory_limit},
        {SP_WRITE_LIMIT, restriction_write_limit},
        {SP_DEADLINE, restriction_user_time_limit},
        {SP_SECURITY_LEVEL, restriction_security_limit},
        {SP_LOAD_RATIO, restriction_load_ratio},
    };
    const int restriction_bindings_count = 6;//hardcoded - bad

    for (int i = 0; i < restriction_bindings_count; ++i) {
        if (arguments.ArgumentExists(restriction_bindings[i].argument)) {
            SetRestriction(restrictions, restriction_bindings[i].restriction, arguments.GetArgument(restriction_bindings[i].argument));
        }
    }
    init_options_from_arguments(options, arguments);

    for (int i = 0; i < arguments.ArgumentCount(SP_OUTPUT_FILE); ++i) {
        options.add_stdoutput(arguments.GetArgument(SP_OUTPUT_FILE, i));
	}
    for (int i = 0; i < arguments.ArgumentCount(SP_ERROR_FILE); ++i) {
        options.add_stderror(arguments.GetArgument(SP_ERROR_FILE, i));
	}
    for (int i = 0; i < arguments.ArgumentCount(SP_INPUT_FILE); ++i) {
        options.add_stdinput(arguments.GetArgument(SP_INPUT_FILE, i));
	}

	std::vector<output_buffer_class *> output_buffers;
	std::vector<output_buffer_class *> error_buffers;
    std::vector<input_buffer_class *> input_buffers;

    secure_runner *secure_runner_instance;
    if (options.delegated) {
        secure_runner_instance = new delegate_runner(arguments.get_program(), options, restrictions);
    } else if (options.session_id.length()){
        secure_runner_instance = new delegate_instance_runner(arguments.get_program(), options, restrictions);
    } else {
        secure_runner_instance = new secure_runner(arguments.get_program(), options, restrictions);
    }

    if (!options.session_id.length()) {
        for (int i = 0; i < options.stdoutput.size(); ++i) {
            output_buffers.push_back(create_output_buffer(options.stdoutput[i], STD_OUTPUT_PIPE));
		}
        for (int i = 0; i < options.stderror.size(); ++i) {
            error_buffers.push_back(create_output_buffer(options.stderror[i], STD_ERROR_PIPE));
		}
        for (int i = 0; i < options.stdinput.size(); ++i) {
            input_buffers.push_back(create_input_buffer(options.stdinput[i]));
		}
        secure_runner_instance->set_pipe(STD_OUTPUT_PIPE, new output_pipe_class(options.session, "stdout", output_buffers));
        secure_runner_instance->set_pipe(STD_ERROR_PIPE, new output_pipe_class(options.session, "stderr", error_buffers));
        secure_runner_instance->set_pipe(STD_INPUT_PIPE, new input_pipe_class(options.session, "stdin", input_buffers));
    }

    secure_runner_instance->run_process();
    report_class rep = secure_runner_instance->get_report();
    std::cout.flush();
    if (!options.hide_report)
        std::cout << GenerateSpawnerReport(rep, options, restrictions);
    if (options.report_file.length())
    {
        std::ofstream fo(options.report_file);
        fo << GenerateSpawnerReport(rep, options, restrictions);
        fo.close();
    }
    delete secure_runner_instance;
	return 0;
}
