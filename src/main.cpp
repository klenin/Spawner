#include <ParseArguments.h>
#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>
#include <inc/compatibility.h>

void init_options_from_arguments(options_class &options, const argument_set_c &argument_set) {
    if (argument_set.argument_exists(SP_WORKING_DIRECTORY))
        options.working_directory = argument_set.get_argument(SP_WORKING_DIRECTORY);

    if (argument_set.argument_exists(SP_SILENT)) {
        options.silent_errors = true;
    }
    if (argument_set.argument_exists(SP_LOGIN)) {
        options.login = argument_set.get_argument(SP_LOGIN);
    }
    if (argument_set.argument_exists(SP_PASSWORD)) {
        options.password = argument_set.get_argument(SP_PASSWORD);
    }
    if (argument_set.argument_exists(SP_HIDE_GUI)) {
        options.hide_gui = true;
    }
    if (argument_set.argument_exists(SP_CMD)) {
	    options.use_cmd = true;
    }
    if (argument_set.argument_exists(SP_DELEGATED)) {
        options.delegated = true;
    } 
    if (argument_set.argument_exists(SP_DELEGATED_SESSION)) {
        options.session_id = argument_set.get_argument(SP_DELEGATED_SESSION);
    } 
    if (argument_set.argument_exists(SP_REPORT_FILE)) {
        options.report_file = argument_set.get_argument(SP_REPORT_FILE);
    }
    if (argument_set.argument_exists(SP_HIDE_REPORT)) {
        options.hide_report = true;
    }
    if (argument_set.argument_exists(SP_DEBUG)) {
        options.debug = true;
    }
    options.hide_gui = true;
}

output_buffer_class *create_output_buffer(const std::string &name, const pipes_t &pipe_type, const size_t buffer_size = 4096) {
	output_buffer_class *output_buffer = NULL;
	if (name == "std") {
        unsigned int color = FOREGROUND_BLUE | FOREGROUND_GREEN;
        if (pipe_type == STD_ERROR_PIPE) {
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
        }
		output_buffer = new output_stdout_buffer_class(4096, color);
	} else if (name.length()) {
		output_buffer = new output_file_buffer_class(name, 4096);
	}
    return output_buffer;
}

input_buffer_class *create_input_buffer(const std::string &name, const size_t buffer_size = 4096) {
    input_buffer_class *input_buffer = NULL;
    if (name == "std") {
        input_buffer = new input_buffer_class(4096);
    } else if (name.length()) {
        input_buffer = new input_file_buffer_class(name, 4096);
    }
    return input_buffer;
}

runner *create_runner(const argument_set_c &argument_set) {
    restrictions_class restrictions;
    options_class options(session_class::base_session);

    options.add_arguments(argument_set.get_program_arguments());

    ReadEnvironmentVariables(options, restrictions);

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
    const uint restriction_bindings_count = 
        sizeof(restriction_bindings)/(sizeof(spawner_arguments) + sizeof(restriction_kind_t));

    for (uint i = 0; i < restriction_bindings_count; ++i) {
        if (argument_set.argument_exists(restriction_bindings[i].argument)) {
            SetRestriction(restrictions, restriction_bindings[i].restriction, argument_set.get_argument(restriction_bindings[i].argument));
        }
    }
    init_options_from_arguments(options, argument_set);

    for (uint i = 0; i < argument_set.get_argument_count(SP_OUTPUT_FILE); ++i) {
        options.add_stdoutput(argument_set.get_argument(SP_OUTPUT_FILE, i));
	}
    for (uint i = 0; i < argument_set.get_argument_count(SP_ERROR_FILE); ++i) {
        options.add_stderror(argument_set.get_argument(SP_ERROR_FILE, i));
	}
    for (uint i = 0; i < argument_set.get_argument_count(SP_INPUT_FILE); ++i) {
        options.add_stdinput(argument_set.get_argument(SP_INPUT_FILE, i));
	}

	std::vector<output_buffer_class *> output_buffers;
	std::vector<output_buffer_class *> error_buffers;
    std::vector<input_buffer_class *> input_buffers;

    secure_runner *secure_runner_instance;
    if (options.delegated) {
        secure_runner_instance = new delegate_runner(argument_set.get_program(), options, restrictions);
    } else if (options.session_id.length()){
        secure_runner_instance = new delegate_instance_runner(argument_set.get_program(), options, restrictions);
    } else {
        secure_runner_instance = new secure_runner(argument_set.get_program(), options, restrictions);
    }

    if (!options.session_id.length()) {
        for (uint i = 0; i < options.stdoutput.size(); ++i) {
            output_buffer_class *buffer = create_output_buffer(options.stdoutput[i], STD_OUTPUT_PIPE);
            if (buffer) {
                output_buffers.push_back(buffer);
            }
		}
        for (uint i = 0; i < options.stderror.size(); ++i) {
            output_buffer_class *buffer = create_output_buffer(options.stderror[i], STD_ERROR_PIPE);
            if (buffer) {
                error_buffers.push_back(buffer);
            }
		}
        for (uint i = 0; i < options.stdinput.size(); ++i) {
            input_buffer_class *buffer = create_input_buffer(options.stdinput[i]);
            if (buffer) {
                input_buffers.push_back(buffer);
            }
		}
        secure_runner_instance->set_pipe(STD_OUTPUT_PIPE, new output_pipe_class(options.session, "stdout", output_buffers));
        secure_runner_instance->set_pipe(STD_ERROR_PIPE, new output_pipe_class(options.session, "stderr", error_buffers));
        secure_runner_instance->set_pipe(STD_INPUT_PIPE, new input_pipe_class(options.session, "stdin", input_buffers));
    }
    return secure_runner_instance;
}



int main(int argc, char *argv[])
{
	//SetConsoleCP(1251);
	//SetConsoleOutputCP(1251);
    std::vector<runner*> runners;
    
	arguments_c arguments(argc, argv);
    if (arguments.get_state() != arguments_state_ok)
    {
        arguments.ShowUsage();
        return 0;
    }

    for (int i = 0; i < arguments.get_argument_set_count(); ++i) {
        runners.push_back(create_runner(arguments.get_argument_set(i)));
    }

    for (int i = 0; i < runners.size(); ++i) {
        runners[i]->run_process_async();
    }

    for (int i = 0; i < runners.size(); ++i) {
        runners[i]->wait_for();
    }
    while (runners.size()) {
        runner *runner_instance = runners[runners.size() - 1];
        report_class rep = runner_instance->get_report();
        options_class options = runner_instance->get_options();
        std::cout.flush();
        if (!options.hide_report || options.report_file.length()) {
            std::string report = GenerateSpawnerReport(
                rep, runner_instance->get_options(), 
                ((secure_runner*)runner_instance)->get_restrictions()
            );
            if (!options.hide_report) {
                std::cout << report;
            }
            if (options.report_file.length())
            {
                std::ofstream fo(options.report_file.c_str());
                fo << GenerateSpawnerReport(rep, options, ((secure_runner*)runner_instance)->get_restrictions());
                fo.close();
            }
        }
        delete runner_instance;
        runners.pop_back();
    }
    /*secure_runner_instance->run_process();
    report_class rep = secure_runner_instance->get_report();
    std::cout.flush();
    if (!options.hide_report)
        std::cout << GenerateSpawnerReport(rep, options, restrictions);
    if (options.report_file.length())
    {
        std::ofstream fo(options.report_file.c_str());
        fo << GenerateSpawnerReport(rep, options, restrictions);
        fo.close();
    }*/
    //delete secure_runner_instance;
	return 0;
}
