#include "spawner.h"
#include <iostream>

spawner_c::spawner_c(int argc, char *argv[]): arguments(argc, argv), state(spawner_state_ok) {
    if (arguments.get_state() != arguments_state_ok)
    {
        arguments.ShowUsage();
        state = spawner_state_show_help;
    }
}

spawner_c::~spawner_c() {
}


void spawner_c::init_options_from_arguments(options_class &options, const argument_set_c &argument_set) {
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

output_buffer_class *spawner_c::create_output_buffer(const std::string &name, const pipes_t &pipe_type, const size_t buffer_size) {
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

input_buffer_class *spawner_c::create_input_buffer(const std::string &name, const size_t buffer_size) {
    input_buffer_class *input_buffer = NULL;
    if (name == "std") {
        input_buffer = new input_stdin_buffer_class(4096);
    } else if (name.length()) {
        input_buffer = new input_file_buffer_class(name, 4096);
    }
    return input_buffer;
}

runner *spawner_c::create_runner(session_class &session, const argument_set_c &argument_set) {
    restrictions_class restrictions;
    options_class options(session);

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

    secure_runner *secure_runner_instance;
    if (options.delegated) {
        secure_runner_instance = new delegate_runner(argument_set.get_program(), options, restrictions);
    } else if (options.session_id.length()){
        secure_runner_instance = new delegate_instance_runner(argument_set.get_program(), options, restrictions);
    } else {
        secure_runner_instance = new secure_runner(argument_set.get_program(), options, restrictions);
    }

    if (!options.session_id.length()) {
        output_pipe_class *output = new output_pipe_class();
        output_pipe_class *error = new output_pipe_class();
        input_pipe_class *input = new input_pipe_class();
        for (uint i = 0; i < options.stdoutput.size(); ++i) {
            output_buffer_class *buffer = create_output_buffer(options.stdoutput[i], STD_OUTPUT_PIPE);
            if (buffer) {
                output->add_output_buffer(buffer);
            }
		}
        for (uint i = 0; i < options.stderror.size(); ++i) {
            output_buffer_class *buffer = create_output_buffer(options.stderror[i], STD_ERROR_PIPE);
            if (buffer) {
                error->add_output_buffer(buffer);
            }
		}
        for (uint i = 0; i < options.stdinput.size(); ++i) {
            input_buffer_class *buffer = create_input_buffer(options.stdinput[i]);
            if (buffer) {
                input->add_input_buffer(buffer);
            }
		}
        secure_runner_instance->set_pipe(STD_OUTPUT_PIPE, output);
        secure_runner_instance->set_pipe(STD_ERROR_PIPE, error);
        secure_runner_instance->set_pipe(STD_INPUT_PIPE, input);
    }
    return secure_runner_instance;
}

void spawner_c::init() {
    for (uint i = 0; i < arguments.get_argument_set_count(); ++i) {
        session_class session(session_class::base_session);
        session << i;
        runners.push_back(create_runner(session, arguments.get_argument_set(i)));
    }
    duplex_buffer_class *buffer = new duplex_buffer_class();
    /*/
    ((input_pipe_class*)runners[0]->get_pipe(STD_INPUT_PIPE))->add_input_buffer(buffer);
    //
    ((output_pipe_class*)runners[1]->get_pipe(STD_OUTPUT_PIPE))->add_output_buffer(buffer);
    //*/
    for (uint i = 0; i < runners.size(); ++i) {
        runners[i]->run_process_async();
    }

    for (uint i = 0; i < runners.size(); ++i) {
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
}

bool spawner_c::run() {
    return 0;
}