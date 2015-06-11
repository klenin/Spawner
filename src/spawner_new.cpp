#include "spawner_new.h"

#include <iostream>

spawner_new_c::spawner_new_c(settings_parser_c &parser)
    : parser(parser)
    , spawner_base_c()
    , options(session_class::base_session)
    , base_options(session_class::base_session)
    , runas(false)
    , order(0)
    , base_initialized(false)
    , control_mode_enabled(false) {
    wait_normal_mutex_.possess();
}

spawner_new_c::~spawner_new_c() {
    for (auto& i : runners) {
        delete i;
    }
    wait_normal_mutex_.release();
}

void spawner_new_c::json_report(runner *runner_instance,
    rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > &writer) {
    writer.StartObject();

    //for {
    report_class runner_report = runner_instance->get_report();
    options_class runner_options = runner_instance->get_options();
    //temporary
#define rapidjson_write(x) (writer.String(a2w(x)))
    rapidjson_write("Application");
    rapidjson_write(runner_report.application_name.c_str());
    rapidjson_write("Arguments");
    writer.StartArray();
    for (size_t i = 0; i < runner_options.get_arguments_count(); ++i) {
        rapidjson_write(runner_options.get_argument(i).c_str());
    }
    writer.EndArray();

    rapidjson_write("Limit");
    writer.StartObject();

    restrictions_class runner_restrictions = ((secure_runner*)runner_instance)->get_restrictions();
    struct {
        char *field;
        unit_t unit;
        degrees_enum degree;
        restriction_kind_t restriction;
    } restriction_items[] = {
        { "Time", unit_time_second, degree_micro, restriction_processor_time_limit },
        { "WallClockTime", unit_time_second, degree_micro, restriction_user_time_limit },
        { "Memory", unit_memory_byte, degree_default, restriction_memory_limit },
        { "SecurityLevel", unit_no_unit, degree_default, restriction_security_limit },
        { "IOBytes", unit_memory_byte, degree_default, restriction_write_limit },
        { "IdlenessTime", unit_time_second, degree_micro, restriction_idle_time_limit },
        { "IdlenessProcessorLoad", unit_no_unit, degree_centi, restriction_load_ratio },
        { NULL, unit_no_unit, degree_default, restriction_max },
    };
    for (int i = 0; restriction_items[i].field; ++i) {
        if (runner_restrictions[restriction_items[i].restriction] == restriction_no_limit) {
            continue;
        }
        rapidjson_write(restriction_items[i].field);
        if (restriction_items[i].degree == degree_default) {
            writer.Uint64(runner_restrictions[restriction_items[i].restriction]);
        }
        else {
            writer.Double((double)convert(
                value_t(restriction_items[i].unit, restriction_items[i].degree),
                value_t(restriction_items[i].unit),
                (long double)runner_restrictions[restriction_items[i].restriction]
            ));
        }
    }
    writer.EndObject();

    rapidjson_write("Options");
    writer.StartObject();
    rapidjson_write("SearchInPath");
    writer.Bool(runner_options.use_cmd);
    writer.EndObject();

    rapidjson_write("Result");
    writer.StartObject();
    struct {
        char *field;
        uint64_t value;
        unit_t unit;
        degrees_enum degree;
    } result_items[] = {
        { "Time", runner_report.processor_time, unit_time_second, degree_micro },
        { "WallClockTime", runner_report.user_time, unit_time_second, degree_micro },
        { "Memory", runner_report.peak_memory_used, unit_memory_byte, degree_default },
        { "BytesWritten", runner_report.write_transfer_count, unit_memory_byte, degree_default },
        { "KernelTime", runner_report.kernel_time, unit_time_second, degree_micro },
        { "ProcessorLoad", runner_report.load_ratio, unit_no_unit, degree_centi },
        { NULL, 0, unit_no_unit, degree_default },
    };
    for (int i = 0; result_items[i].field; ++i) {
        rapidjson_write(result_items[i].field);
        if (result_items[i].degree == degree_default) {
            writer.Uint64(result_items[i].value);
        }
        else {
            writer.Double((double)convert(
                value_t(result_items[i].unit, result_items[i].degree),
                value_t(result_items[i].unit),
                (long double)result_items[i].value
            ));
        }
    }
    rapidjson_write("WorkingDirectory");
    rapidjson_write(runner_report.working_directory.c_str());
    writer.EndObject();

    rapidjson_write("StdOut");
    writer.StartArray();
    for (uint i = 0; i < runner_options.stdoutput.size(); ++i) {
        rapidjson_write(runner_options.stdoutput[i].c_str());
    }
    writer.EndArray();
    rapidjson_write("StdErr");
    writer.StartArray();
    for (uint i = 0; i < runner_options.stderror.size(); ++i) {
        rapidjson_write(runner_options.stderror[i].c_str());
    }
    writer.EndArray();
    rapidjson_write("StdIn");
    writer.StartArray();
    for (uint i = 0; i < runner_options.stdinput.size(); ++i) {
        rapidjson_write(runner_options.stdinput[i].c_str());
    }
    writer.EndArray();

    rapidjson_write("CreateProcessMethod");
    rapidjson_write(options.login == "" ? "CreateProcess" : "WithLogon");
    rapidjson_write("UserName");
    writer.String(runner_report.login.c_str());
    rapidjson_write("TerminateReason");
    rapidjson_write(get_terminate_reason(runner_report.terminate_reason).c_str());
    rapidjson_write("ExitCode");
    writer.Uint(runner_report.exit_code);
    rapidjson_write("ExitStatus");
    rapidjson_write(ExitCodeToString(runner_report.exit_code).c_str());
    rapidjson_write("SpawnerError");
    writer.StartArray();
    std::vector<std::string> errors;
    errors.push_back(get_error_text());
    for (auto& error : errors) {
        rapidjson_write(error.c_str());
    }
    writer.EndArray();
    writer.EndObject();
}

int spawner_new_c::get_normal_index_(const std::string& message) {
    int normal_index = -1;
    try {
        normal_index = stoi(message);
    // std::invalid_argument std::out_of_range
    } catch (...) {
        normal_index = -1;
    }

    if (normal_index == 0) {
        return 0;
    } else if (normal_index < 1 || normal_index > runners.size() - 1) {
        normal_index = -1;
    }

    if (normal_index != -1) {
        int normal_runner_index = normal_to_runner_index_(normal_index);
        auto status = runners[normal_runner_index]->get_process_status();
        if (status != process_still_active
         && status != process_suspended
         && status != process_not_started) {
            normal_index = -1;
        }
    }
    return normal_index;
}

int spawner_new_c::normal_to_runner_index_(int normal_index) {
    // normal_index must be valid here
    int normal_runner_index = normal_index - 1;
    if (normal_runner_index >= controller_index_) {
        normal_runner_index++;
    }
    PANIC_IF(normal_runner_index <= 0 || normal_runner_index >= runners.size());
    return normal_runner_index;
}

void spawner_new_c::process_controller_message_(const std::string& message, output_pipe_c* pipe) {
    const int hash_pos = message.find_first_of('#');
    if (hash_pos == std::string::npos) {
        PANIC("no hash prefix in controller message");
    } else if (hash_pos == 0) {
        PANIC("missing header in controller message");
    }
    char control_letter = message[hash_pos - 1];
    bool send_to_normal = false;

    const int normal_index = get_normal_index_(message);
    if (normal_index == -1) {
        const std::string error_message = message.substr(0, hash_pos) + "I#\n";
        controller_buffer_->write(error_message.c_str(), error_message.size());
        send_to_normal = true;
    }
    if (normal_index == 0) {
        // this is a message to spawner
    } else switch (control_letter) {
    case 'W': {
        wait_normal_mutex_.lock();
        awaited_normals_[normal_index - 1] = true;
        int runner_index = normal_to_runner_index_(normal_index);
        runners[runner_index]->resume();
        wait_normal_mutex_.unlock();
        break;
    }
    default:
        send_to_normal = true;
        break;
    }

    for (uint i = 0; i < pipe->output_buffers.size(); ++i) {
        auto& buffer = pipe->output_buffers[i];
        auto at_index = buffer_to_runner_index_.find(buffer);
        if (at_index == buffer_to_runner_index_.end()) {
            buffer->write(message.c_str(), message.size());
        } else if (send_to_normal && at_index->second == normal_index) {
            buffer->write(message.c_str() + hash_pos + 1, message.size() - hash_pos - 1);
        }
    }
}

void spawner_new_c::process_normal_message_(const std::string& message, output_pipe_c* pipe, int normal_index) {
    std::string mod_message = std::to_string(normal_index) + "#" + message;
    for (uint i = 0; i < pipe->output_buffers.size(); ++i) {
        pipe->output_buffers[i]->write(mod_message.c_str(), mod_message.size());
    }
    wait_normal_mutex_.lock();
    if (awaited_normals_[normal_index - 1]) {
        int runner_index = normal_to_runner_index_(normal_index);
        awaited_normals_[normal_index - 1] = false;
        runners[runner_index]->suspend();
    } else {
        // it hasn't been waited for, but sent a message. what do?
    }
    wait_normal_mutex_.unlock();
}

void spawner_new_c::setup_stream_(const std::string& stream_str, pipes_t this_pipe_type, runner* this_runner) {
    size_t pos = stream_str.find(".");
    // malformed argument
    PANIC_IF(pos == std::string::npos);
    size_t index = stoi(stream_str.substr(1, pos - 1), nullptr, 10);
    std::string stream = stream_str.substr(pos + 1);
    // invalid index
    PANIC_IF(index > runners.size() || index < 0);
    pipes_t other_pipe_type;
    if (stream == "stderr") {
        other_pipe_type = STD_ERROR_PIPE;
    }
    else if (stream == "stdin") {
        other_pipe_type = STD_INPUT_PIPE;
    }
    else if (stream == "stdout") {
        other_pipe_type = STD_OUTPUT_PIPE;
    }
    else {
        PANIC("invalid stream name");
    }
    runner *target_runner = runners[index];
    std::shared_ptr<input_pipe_c> input_pipe = nullptr;
    std::shared_ptr<output_pipe_c> output_pipe = nullptr;
    pipes_t out_pipe_type = STD_ERROR_PIPE;
    runner* out_pipe_runner = nullptr;
    runner* in_pipe_runner = nullptr;

    if (this_pipe_type == STD_INPUT_PIPE && other_pipe_type != STD_INPUT_PIPE) {

        input_pipe = std::static_pointer_cast<input_pipe_c>(this_runner->get_pipe(this_pipe_type));
        output_pipe = std::static_pointer_cast<output_pipe_c>(target_runner->get_pipe(other_pipe_type));
        out_pipe_type = other_pipe_type;
        out_pipe_runner = target_runner;
        in_pipe_runner = this_runner;

    } else if (this_pipe_type != STD_INPUT_PIPE && other_pipe_type == STD_INPUT_PIPE) {

        input_pipe = std::static_pointer_cast<input_pipe_c>(target_runner->get_pipe(other_pipe_type));
        output_pipe = std::static_pointer_cast<output_pipe_c>(this_runner->get_pipe(this_pipe_type));
        out_pipe_type = this_pipe_type;
        out_pipe_runner = this_runner;
        in_pipe_runner = target_runner;
    } else {
        PANIC("invalid pipe mapping");
    }

    std::shared_ptr<duplex_buffer_c> buffer = std::make_shared<duplex_buffer_c>();
    input_pipe->add_input_buffer(buffer);
    output_pipe->add_output_buffer(buffer);

    int out_runner_index = -1;
    int in_runner_index = -1;
    for (int i = 0; i < runners.size(); i++) {
        if (runners[i] == out_pipe_runner) {
            out_runner_index = i;
        }
        if (runners[i] == in_pipe_runner) {
            in_runner_index = i;
        }
    }

    if (out_runner_index == controller_index_) {
        int index = in_runner_index;
        if (index > controller_index_) {
            index--;
        }
        buffer_to_runner_index_[buffer] = index + 1;
    }

    if (control_mode_enabled
     && out_pipe_type == STD_OUTPUT_PIPE
     && !output_pipe->process_message) {

        if (out_pipe_runner->get_options().controller) {
            output_pipe->process_message = [=](std::string& message, output_pipe_c* pipe) {
                process_controller_message_(message, pipe);
            };
        } else {
            int index = out_runner_index;
            if (index > controller_index_) {
                index--;
            }
            output_pipe->process_message = [=](std::string& message, output_pipe_c* pipe) {
                process_normal_message_(message, pipe, index + 1);
            };
        }
    }
}

bool spawner_new_c::init() {
    if (!init_runner() || !runners.size()) {
        return false;
    }
    for (int i = 0; i < runners.size(); i++) {
        if (runners[i]->get_options().controller) {
            // there must be only one controller process
            PANIC_IF(controller_index_ != -1);
            controller_index_ = i;
            control_mode_enabled = true;
            controller_buffer_ = std::make_shared<pipe_buffer_c>(runners[i]->get_input_pipe());
        }
    }
    if (controller_index_ != -1) {
        awaited_normals_.resize(runners.size() - 1);
        for (int i = 0; i < runners.size(); i++) {
            if (i != controller_index_) {
                runners[i]->start_suspended = true;
            }
        }
    }

    for (auto& runner : runners) {
        options_class runner_options = runner->get_options();
        struct {
            std::vector<std::string> &streams;
            pipes_t pipe_type;
        } streams[] = {
            { runner_options.stdinput, STD_INPUT_PIPE },
            { runner_options.stdoutput, STD_OUTPUT_PIPE },
            { runner_options.stderror, STD_ERROR_PIPE },
        };
        for (auto& stream_item : streams) {
            for (auto& stream_str : stream_item.streams) {
                PANIC_IF(stream_str.size() == 0);
                if (stream_str[0] != '*') {
                    continue;
                }
                setup_stream_(stream_str, stream_item.pipe_type, runner);
            }
        }
    }
    return true;
}

bool spawner_new_c::init_runner() {
    if (!parser.get_program().length()) {
        if (base_initialized) {
            return false;
        }
        base_options = options;
        base_restrictions = restrictions;
        base_initialized = true;
        return true;
        //throw exception
    }
    runner *secure_runner_instance;
    options.session << order++ << time(NULL) << runner::get_current_time();
    options.add_arguments(parser.get_program_arguments());
    if (options.login.length())
    {
        secure_runner_instance = new delegate_runner(parser.get_program(), options, restrictions);
    }
    else
    {
        secure_runner_instance = new secure_runner(parser.get_program(), options, restrictions);
    }

    {//if (!options.session_id.length()) {
        std::shared_ptr<output_pipe_c> output = std::make_shared<output_pipe_c>();
        std::shared_ptr<output_pipe_c> error = std::make_shared<output_pipe_c>();
        std::shared_ptr<input_pipe_c> input = std::make_shared<input_pipe_c>();
        for (uint i = 0; i < options.stdoutput.size(); ++i) {
            std::shared_ptr<output_buffer_c> buffer = create_output_buffer(options.stdoutput[i], STD_OUTPUT_PIPE);
            if (buffer) {
                output->add_output_buffer(buffer);
            }
        }
        for (uint i = 0; i < options.stderror.size(); ++i) {
            std::shared_ptr<output_buffer_c> buffer = create_output_buffer(options.stderror[i], STD_ERROR_PIPE);
            if (buffer) {
                error->add_output_buffer(buffer);
            }
        }
        for (uint i = 0; i < options.stdinput.size(); ++i) {
            std::shared_ptr<input_buffer_c> buffer = create_input_buffer(options.stdinput[i]);
            if (buffer) {
                input->add_input_buffer(buffer);
            }
        }
        secure_runner_instance->set_pipe(STD_OUTPUT_PIPE, output);
        secure_runner_instance->set_pipe(STD_ERROR_PIPE, error);
        secure_runner_instance->set_pipe(STD_INPUT_PIPE, input);
    }
    runners.push_back(secure_runner_instance);
    return true;
}

void spawner_new_c::run() {
    begin_report();
    for (auto& i : runners) {
        i->run_process_async();
    }
    for (auto& i : runners) {
        i->wait_for();
    }
    print_report();
}

void spawner_new_c::print_report() {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > report_writer(s);
    report_writer.StartArray();
    for (auto i = runners.begin(); i != runners.end(); i++) {
        report_class rep = (*i)->get_report();
        options_class options_item = (*i)->get_options();
        std::cout.flush();
        if (!options_item.hide_report || options_item.report_file.length()) {
            std::string report;
            json_report(*i, report_writer);

            if (options_item.login.length() > 0)
            {
                HANDLE hIn = OpenFileMappingA(
                    FILE_MAP_ALL_ACCESS,
                    FALSE,
                    options_item.shared_memory.c_str()
                    );

                LPTSTR pRep = (LPTSTR)MapViewOfFile(
                    hIn,
                    FILE_MAP_ALL_ACCESS,
                    0,
                    0,
                    options_class::SHARED_MEMORY_BUF_SIZE
                    );

                report = pRep;

                UnmapViewOfFile(pRep);

                CloseHandle(hIn);
            }

            if (report.length() == 0)
            {
                if (!options_item.json)
                {
                    report = GenerateSpawnerReport(
                        rep, options_item,
                        (*i)->get_restrictions()
                        );
                }
                else
                {
                    rapidjson::StringBuffer sub_report;
                    rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > report_item_writer(sub_report);
                    report_item_writer.StartArray();
                    json_report(*i, report_item_writer);
                    report_item_writer.EndArray();
                    report = sub_report.GetString();
                }
            }

            if (options_item.delegated)
            {
                HANDLE hOut = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, options_item.shared_memory.c_str());
                LPCSTR pRep = (LPTSTR)MapViewOfFile(hOut, FILE_MAP_ALL_ACCESS, 0, 0, options_class::SHARED_MEMORY_BUF_SIZE);

                memcpy((PVOID)pRep, report.c_str(), sizeof(char) * report.length());

                UnmapViewOfFile(pRep);

                CloseHandle(hOut);
            }
            else
            {
                if (!options_item.hide_report && runners.size() == 1)
                {
                    std::cout << report;
                }

                if (options_item.report_file.length())
                {
                    std::ofstream fo(options_item.report_file.c_str());
                    fo << report;
                    fo.close();
                }
            }
        }
    }
    report_writer.EndArray();
    if (base_options.json && runners.size() > 1) {
        std::string report = s.GetString();
        if (base_options.report_file.length()) {
            std::ofstream fo(base_options.report_file.c_str());
            fo << report;
            fo.close();
        }
        if (!base_options.hide_report) {
            std::cout << report;
        }
    }
}

void spawner_new_c::on_separator(const std::string &_) {
    init_runner();
    parser.clear_program_parser();
    options = options_class(base_options);
    restrictions = restrictions_class(base_restrictions);
}

void spawner_new_c::init_arguments() {
    parser.set_dividers(c_lst("=", ":").vector());

    console_argument_parser_c *console_default_parser = new console_argument_parser_c();
    environment_variable_parser_c *environment_default_parser = new environment_variable_parser_c();

    console_default_parser->add_argument_parser(c_lst(short_arg("tl")),
        environment_default_parser->add_argument_parser(c_lst("SP_TIME_LIMIT"), new microsecond_argument_parser_c(restrictions[restriction_processor_time_limit]))
        )->set_description("Time limit");
    console_default_parser->add_argument_parser(c_lst(short_arg("d")),
        environment_default_parser->add_argument_parser(c_lst("SP_DEADLINE"), new microsecond_argument_parser_c(restrictions[restriction_user_time_limit]))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("s")),
        environment_default_parser->add_argument_parser(c_lst("SP_SECURITY_LEVEL"), new bool_restriction_argument_parser_c(restrictions[restriction_security_limit]))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("ml")),
        environment_default_parser->add_argument_parser(c_lst("SP_MEMORY_LIMIT"), new byte_argument_parser_c(restrictions[restriction_memory_limit]))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("wl")),
        environment_default_parser->add_argument_parser(c_lst("SP_WRITE_LIMIT"), new byte_argument_parser_c(restrictions[restriction_write_limit]))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("lr")),
        environment_default_parser->add_argument_parser(c_lst("SP_LOAD_RATIO"), new percent_argument_parser_c(restrictions[restriction_load_ratio]))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("y")),
        environment_default_parser->add_argument_parser(c_lst("SP_IDLE_TIME_LIMIT"), new microsecond_argument_parser_c(restrictions[restriction_idle_time_limit]))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("u")),
        environment_default_parser->add_argument_parser(c_lst("SP_USER"), new string_argument_parser_c(options.login))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("p")),
        environment_default_parser->add_argument_parser(c_lst("SP_PASSWORD"), new string_argument_parser_c(options.password))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("sr")),
        environment_default_parser->add_argument_parser(c_lst("SP_REPORT_FILE"), new string_argument_parser_c(options.report_file))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("env")),
        environment_default_parser->add_argument_parser(c_lst("SP_CLEAR_ENV"), new environment_mode_argument_parser_c(options.environmentMode))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("D")),
        new options_callback_argument_parser_c(&options, &options_class::add_environment_variable)
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("so"), long_arg("out")),
        environment_default_parser->add_argument_parser(c_lst("SP_OUTPUT_FILE"), new options_callback_argument_parser_c(&options, &options_class::add_stdoutput))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("i"), long_arg("in")),
        environment_default_parser->add_argument_parser(c_lst("SP_INPUT_FILE"), new options_callback_argument_parser_c(&options, &options_class::add_stdinput))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("e"), short_arg("se"), long_arg("err")),
        environment_default_parser->add_argument_parser(c_lst("SP_ERROR_FILE"), new options_callback_argument_parser_c(&options, &options_class::add_stderror))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("runas"), long_arg("delegated")),
        environment_default_parser->add_argument_parser(c_lst("SP_RUNAS"), new boolean_argument_parser_c(options.delegated))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("ho")),
        environment_default_parser->add_argument_parser(c_lst("SP_HIDE_OUTPUT"), new boolean_argument_parser_c(options.hide_output))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("hr")),
        environment_default_parser->add_argument_parser(c_lst("SP_HIDE_REPORT"), new boolean_argument_parser_c(options.hide_report))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("sw")),
        environment_default_parser->add_argument_parser(c_lst("SP_SHOW_WINDOW"), new inverted_boolean_argument_parser_c(options.hide_gui))
        );

    console_default_parser->add_argument_parser(c_lst(long_arg("debug")),
        environment_default_parser->add_argument_parser(c_lst("SP_DEBUG"), new boolean_argument_parser_c(options.debug))
        );
    console_default_parser->add_flag_parser(c_lst(long_arg("cmd"), short_arg("cmd"), long_arg("systempath")),
        environment_default_parser->add_argument_parser(c_lst("SP_SYSTEM_PATH"), new boolean_argument_parser_c(options.use_cmd))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("wd")),
        environment_default_parser->add_argument_parser(c_lst("SP_DIRECTORY"), new string_argument_parser_c(options.working_directory))
        );

    console_default_parser->add_flag_parser(c_lst(long_arg("json")),
        environment_default_parser->add_argument_parser(c_lst("SP_JSON"), new boolean_argument_parser_c(options.json))
        );

    console_default_parser->add_argument_parser(c_lst(long_arg("separator")),
        environment_default_parser->add_argument_parser(c_lst("SP_SEPARATOR"), new callback_argument_parser_c<settings_parser_c*, void(settings_parser_c::*)(const std::string&)>(&parser, &settings_parser_c::set_separator))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("process-count")), new time_argument_parser_c<degree_default>(restrictions[restriction_processes_count_limit]));

    console_default_parser->add_argument_parser(c_lst(long_arg("shared-memory")),
        environment_default_parser->add_argument_parser(c_lst("SP_SHARED_MEMORY"), new string_argument_parser_c(options.shared_memory))
        );

    console_default_parser->add_flag_parser(c_lst(long_arg("controller")),
        new boolean_argument_parser_c(options.controller));

    console_default_parser->add_flag_parser(c_lst(SEPARATOR_ARGUMENT), new callback_argument_parser_c<spawner_new_c*, void(spawner_new_c::*)(const std::string&)>(&(*this), &spawner_new_c::on_separator));

    //ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(long_arg("program")), c_lst("SP_PROGRAM"),   options.session_id, STRING_CONVERT);

    parser.add_parser(console_default_parser);
    parser.add_parser(environment_default_parser);
}

std::string spawner_new_c::help() {
    return parser.help();
}
