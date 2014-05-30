#include "spawner.h"
#include <json/json.h>
#include <inc/uconvert.h>
#include <iostream>
#include <regex>

spawner_c::spawner_c(int argc, char *argv[]): arguments(argc, argv), state(spawner_state_ok) {
    if (arguments.get_state() != arguments_state_ok) {
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
    if (argument_set.argument_exists(SP_JSON)) {
        options.json = true;
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
    if (argument_set.argument_exists(SP_HIDE_OUTPUT)) {
        options.hide_output = argument_set.get_argument(SP_HIDE_OUTPUT) == std::string("1");
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
    } else if (name[0] == '*') {
	} else if (name.length()) {
		output_buffer = new output_file_buffer_class(name, 4096);
	}
    return output_buffer;
}

input_buffer_class *spawner_c::create_input_buffer(const std::string &name, const size_t buffer_size) {
    input_buffer_class *input_buffer = NULL;
    if (name == "std") {
        input_buffer = new input_stdin_buffer_class(4096);
    } else if (name[0] == '*') {
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
        {SP_IDLE_TIME_LIMIT, restriction_idle_time_limit},
    };
    const uint restriction_bindings_count = 
        sizeof(restriction_bindings)/(sizeof(spawner_arguments) + sizeof(restriction_kind_t));

    for (uint i = 0; i < restriction_bindings_count; ++i) {
        if (argument_set.argument_exists(restriction_bindings[i].argument)) {
            SetRestriction(restrictions, restriction_bindings[i].restriction, argument_set.get_argument(restriction_bindings[i].argument));
        }
    }
    init_options_from_arguments(options, argument_set);

    bool met_stdout = false;
    for (uint i = 0; i < argument_set.get_argument_count(SP_OUTPUT_FILE); ++i) {
        if (!options.hide_output || argument_set.get_argument(SP_OUTPUT_FILE, i) != "std") {
            options.add_stdoutput(argument_set.get_argument(SP_OUTPUT_FILE, i));
        }
        if (argument_set.get_argument(SP_OUTPUT_FILE, i) == "std") {
            met_stdout = true;
        }
        if (!argument_set.get_argument(SP_OUTPUT_FILE, i).length()) {
            met_stdout = false;
            options.clear_stdoutput();
        }
	}
    if (!options.hide_output && !met_stdout) {
        options.add_stdoutput("std");
    }
    for (uint i = 0; i < argument_set.get_argument_count(SP_ERROR_FILE); ++i) {
        if (!options.hide_output || argument_set.get_argument(SP_ERROR_FILE, i) != "std") {
            options.add_stderror(argument_set.get_argument(SP_ERROR_FILE, i));
        }
        if (!argument_set.get_argument(SP_ERROR_FILE, i).length()) {
            options.clear_stderror();
        }
	}
    for (uint i = 0; i < argument_set.get_argument_count(SP_INPUT_FILE); ++i) {
        options.add_stdinput(argument_set.get_argument(SP_INPUT_FILE, i));
        if (!argument_set.get_argument(SP_INPUT_FILE, i).length()) {
            options.clear_stdinput();
        }
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
    if (state != spawner_state_ok) {
        return;
    }
    for (uint i = 0; i < arguments.get_argument_set_count(); ++i) {
        session_class session(session_class::base_session);
        session << i;
        runners.push_back(create_runner(session, arguments.get_argument_set(i)));
    }
    for (uint i = 0; i < runners.size(); ++i) {
        runner *r = runners[i];
        options_class options = r->get_options();
        std::regex stdin_regex("^\\*([[:digit:]]+)\\.(stdout|stderr)$");
        std::regex stdout_regex("^\\*([[:digit:]]+)\\.stdin$");
        for (uint j = 0; j < options.stdinput.size(); ++j) {
            std::cmatch result;
            if (regex_search(options.stdinput[j].c_str(), result, stdin_regex)) {
                uint id = atoi(result[1].str().c_str());
                if (id >= runners.size()) {
                    //fail with error
                    throw;
                }
                pipes_t output_pipe = result[2].str() == "stdout" ? STD_OUTPUT_PIPE : STD_ERROR_PIPE;
                runner *target_runner = runners[id];

                duplex_buffer_class *buffer = new duplex_buffer_class();
                static_cast<input_pipe_class*>(r->get_pipe(STD_INPUT_PIPE))->add_input_buffer(buffer);
                static_cast<output_pipe_class*>(target_runner->get_pipe(output_pipe))->add_output_buffer(buffer);
                
            }
        }
        for (uint j = 0; j < options.stdoutput.size(); ++j) {
            std::cmatch result;
            if (regex_search(options.stdoutput[j].c_str(), result, stdout_regex)) {
                uint id = atoi(result[1].str().c_str());
                if (id >= runners.size()) {
                    //fail with error
                    throw;
                }
                duplex_buffer_class *buffer = new duplex_buffer_class();
                static_cast<input_pipe_class*>(runners[id]->get_pipe(STD_INPUT_PIPE))->add_input_buffer(buffer);
                static_cast<output_pipe_class*>(r->get_pipe(STD_OUTPUT_PIPE))->add_output_buffer(buffer);
                
            }
        }
        for (uint j = 0; j < options.stderror.size(); ++j) {
            std::cmatch result;
            if (regex_search(options.stderror[j].c_str(), result, stdout_regex)) {
                uint id = atoi(result[1].str().c_str());
                if (id >= runners.size()) {
                    //fail with error
                    throw;
                }
                duplex_buffer_class *buffer = new duplex_buffer_class();
                static_cast<input_pipe_class*>(runners[id]->get_pipe(STD_INPUT_PIPE))->add_input_buffer(buffer);
                static_cast<output_pipe_class*>(r->get_pipe(STD_ERROR_PIPE))->add_output_buffer(buffer);
                
            }
        }
    }
}

bool spawner_c::run() {
    if (state != spawner_state_ok) {
        return 0;
    }
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
            std::string report;
            if (!options.json) {
                report = GenerateSpawnerReport(
                    rep, runner_instance->get_options(), 
                    ((secure_runner*)runner_instance)->get_restrictions()
                );
            } else {
                report = json_report(runner_instance);
            }
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
    return 0;
}

Json::Value unit_to_json(restriction_t real_value, double value, const std::string &units) {
    if (real_value == restriction_no_limit) {
        return "Infinity";
    }
    Json::Value object(Json::objectValue);
    object["real_value"] = real_value;
    object["value"] = value;
    object["units"] = units;
    return object;
}

std::string spawner_c::json_report(runner *runner_instance) {
    Json::Value object(Json::objectValue);
    report_class rep = runner_instance->get_report();
    options_class options = runner_instance->get_options();
    restrictions_class restrictions = ((secure_runner*)runner_instance)->get_restrictions();

    object["Application"]           = rep.application_name;
    object["Parameters"]            = Json::Value(Json::arrayValue);
    for (size_t i = 0; i < options.get_arguments_count(); ++i) {
        object["Parameters"].append(options.get_argument(i));
    }
    object["SecurityLevel"]         = (restrictions.get_restriction(restriction_security_limit) == restriction_limited);
    object["CreateProcessMethod"]   = (options.login==""?"CreateProcess":"WithLogon");
    object["UserName"]              = rep.login;

    object["UserTimeLimit"] = unit_to_json(
        restrictions.get_restriction(restriction_processor_time_limit),
        convert(
            value_t(unit_time_second, degree_milli), 
            value_t(unit_time_second), 
            (long double)restrictions.get_restriction(restriction_processor_time_limit)
            ),
        unit_name(unit_time_second)
    );
    object["Deadline"] = unit_to_json(
        restrictions.get_restriction(restriction_user_time_limit),
        convert(
            value_t(unit_time_second, degree_milli),
            value_t(unit_time_second),
            (long double)restrictions.get_restriction(restriction_user_time_limit)
            ),
        unit_name(unit_time_second)
    );
    object["MemoryLimit"] = unit_to_json(
        restrictions.get_restriction(restriction_memory_limit),
        convert(
            value_t(unit_memory_byte),
            value_t(unit_memory_byte, degree_mega),
            (long double)restrictions.get_restriction(restriction_memory_limit)
            ),
        degree_short_name(degree_mega) + unit_short_name(unit_memory_byte)
    );
    object["WriteLimit"] = unit_to_json(
        restrictions.get_restriction(restriction_write_limit),
        convert(
            value_t(unit_memory_byte),
            value_t(unit_memory_byte, degree_mega),
            (long double)restrictions.get_restriction(restriction_write_limit)
            ),
        degree_short_name(degree_mega) + unit_short_name(unit_memory_byte)
    );
    object["UserTime"] = unit_to_json(
        rep.processor_time,
        convert(
            value_t(unit_time_second, degree_micro),
            value_t(unit_time_second),
            (long double)rep.processor_time/10.0
            ),
        unit_name(unit_time_second)
    );
    object["PeakMemoryUsed"] = unit_to_json(
        rep.peak_memory_used,
        convert(
            value_t(unit_memory_byte),
            value_t(unit_memory_byte, degree_mega),
            (long double)rep.peak_memory_used
            ),
        degree_short_name(degree_mega) + unit_short_name(unit_memory_byte)
    );
    object["Written"] = unit_to_json(
        rep.write_transfer_count,
        convert(
            value_t(unit_memory_byte),
            value_t(unit_memory_byte, degree_mega),
            (long double)rep.write_transfer_count
            ),
        degree_short_name(degree_mega) + unit_short_name(unit_memory_byte)
    );
    //object["Deadline"]              = convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit);
    //object["MemoryLimit"]           = convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit);
    //object["WriteLimit"]            = convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit);
    //object["UserTime"]              = convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.processor_time/10.0, " (sec)");
    //object["PeakMemoryUsed"]        = convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (Mb)");
    //object["Written"]               = convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.write_transfer_count, " (Mb)");
    object["TerminateReason"]       = get_terminate_reason(rep.terminate_reason);
	object["ExitCode"]              = rep.exit_code;
	object["ExitStatus"]            = ExitCodeToString(rep.exit_code);
    object["SpawnerError"]          = error_list::pop_error();
    return object.toStyledString();
}