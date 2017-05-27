#include "spawner_new.h"

#include <iostream>

#include "inc/logger.h"


spawner_new_c::spawner_new_c(settings_parser_c &parser)
    : parser(parser)
    , spawner_base_c()
    , options(session_class::base_session)
    , base_options(session_class::base_session)
    , runas(false)
    , order(0)
    , base_initialized(false)
    , control_mode_enabled(false) {
}

spawner_new_c::~spawner_new_c() {
    for (auto i : runners) {
        i->wait_for();
        delete i;
    }
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
        const char *field;
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
        { nullptr, unit_no_unit, degree_default, restriction_max },
    };
    for (int i = 0; restriction_items[i].field; ++i) {
        if (!runner_restrictions.check_restriction(restriction_items[i].restriction)) {
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
        const char *field;
        uint64_t value;
        unit_t unit;
        degrees_enum degree;
    } result_items[] = {
        { "Time", runner_report.processor_time, unit_time_second, degree_micro },
        { "WallClockTime", runner_report.user_time, unit_time_second, degree_micro },
        { "Memory", runner_report.peak_memory_used, unit_memory_byte, degree_default },
        { "BytesWritten", runner_report.write_transfer_count, unit_memory_byte, degree_default },
        { "KernelTime", runner_report.kernel_time, unit_time_second, degree_micro },
        { "ProcessorLoad", (uint64_t)(runner_report.load_ratio * 100), unit_no_unit, degree_centi },
        { nullptr, 0, unit_no_unit, degree_default },
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
    for (const auto& i : runner_options.stdoutput) {
        rapidjson_write(i.original.c_str());
    }
    writer.EndArray();
    rapidjson_write("StdErr");
    writer.StartArray();
    for (const auto& i : runner_options.stderror) {
        rapidjson_write(i.original.c_str());
    }
    writer.EndArray();
    rapidjson_write("StdIn");
    writer.StartArray();
    for (const auto& i : runner_options.stdinput) {
        rapidjson_write(i.original.c_str());
    }
    writer.EndArray();

    rapidjson_write("CreateProcessMethod");
    rapidjson_write(options.login.empty() ? "CreateProcess" : "WithLogon");
    rapidjson_write("UserName");
    writer.String(runner_report.login.c_str());
    rapidjson_write("TerminateReason");
    rapidjson_write(get_terminate_reason(runner_report.terminate_reason).c_str());
    rapidjson_write("ExitCode");
    writer.Uint(runner_report.exit_code);
    rapidjson_write("ExitStatus");
    rapidjson_write(ExtractExitStatus(runner_report).c_str());
    rapidjson_write("SpawnerError");
    writer.StartArray();
    std::vector<std::string> errors;
    errors.push_back(get_error_text());
    for (const auto& error : errors) {
        rapidjson_write(error.c_str());
    }
    writer.EndArray();
    writer.EndObject();
}

int spawner_new_c::get_agent_index_(const std::string& message) {
    int agent_index = -1;
    try {
        agent_index = stoi(message);
    // std::invalid_argument std::out_of_range
    } catch (...) {
        agent_index = -1;
    }

    if (agent_index == 0) {
        return 0;
    } else if (agent_index < 1 || agent_index > int(runners.size() - 1)) {
        agent_index = -1;
    }

    if (agent_index != -1) {
        int agent_runner_index = agent_to_runner_index_(agent_index);
        auto status = runners[agent_runner_index]->get_process_status();
        if (status != process_still_active
         && status != process_suspended
         && status != process_not_started) {
            agent_index = -1;
        }
    }
    return agent_index;
}

int spawner_new_c::agent_to_runner_index_(int agent_index) {
    // agent_index must be valid here
    int agent_runner_index = agent_index - 1;
    if (agent_runner_index >= controller_index_) {
        agent_runner_index++;
    }
    PANIC_IF(agent_runner_index <= 0 || agent_runner_index >= (int)runners.size());
    return agent_runner_index;
}

void spawner_new_c::process_controller_message_(const std::string& message) {
    static_cast<secure_runner*>(runners[controller_index_])->prolong_time_limits();
    const int hash_pos = message.find_first_of('#');
    if (hash_pos == std::string::npos) {
        PANIC("no hash prefix in controller message");
    } else if (hash_pos == 0) {
        PANIC("missing header in controller message");
    }
    auto control_letter = message[hash_pos - 1];
    auto send_to_agent = false;

    const auto agent_index = get_agent_index_(message);
    auto runner_index = -1;
    if (agent_index == -1) {
        const auto error_message = message.substr(0, hash_pos) + "I#\n";
        controller_broadcaster_->write(error_message.c_str(), error_message.size());
        send_to_agent = true;
    } else if (agent_index == 0) {
        // this is a message to spawner
    }
    else {
        runner_index = agent_to_runner_index_(agent_index);
        switch (control_letter) {
        case 'W': {
            wait_agent_mutex_.lock();
            awaited_agents_[agent_index - 1] = true;
            runners[runner_index]->resume();
            wait_agent_mutex_.unlock();
            break;
        }
        case 'S': {
            static_cast<secure_runner*>(runners[runner_index])->force_stop = true;
            break;
        }
        default:
            send_to_agent = true;
            break;
        }
    }

    //TODO: what if agent_index == -1?
    if (runner_index != -1) {
        auto pipe = runners[runner_index]->get_pipe(std_stream_input);
        pipe->write(message.c_str() + hash_pos + 1, message.size() - hash_pos - 1);
    }
}

void spawner_new_c::process_agent_message_(const std::string& message, int agent_index) {
    std::string mod_message = std::to_string(agent_index) + "#" + message;
    auto runner = runners[agent_to_runner_index_(agent_index)];
    runner->get_pipe(std_stream_input)->write(mod_message.c_str(), mod_message.size());
    wait_agent_mutex_.lock();
    if (awaited_agents_[agent_index - 1]) {
        awaited_agents_[agent_index - 1] = false;
        runner->suspend();
        static_cast<secure_runner*>(runner)->prolong_time_limits();
    } else {
        // it hasn't been waited for, but sent a message. what do?
    }
    wait_agent_mutex_.unlock();
}

void spawner_new_c::setup_stream_in_control_mode_(runner* runner, multipipe_ptr pipe) {
    if (pipe->process_message_is_custom()) {
        return;
    }
    if (runner->get_options().controller) {
        pipe->set_custom_process_message([=](const char* buffer, size_t count) {
            string message(buffer, count);
            process_controller_message_(message);
        });
    }
    else {
        auto index = runner->get_index();
        if (index > controller_index_) {
            index--;
        }
        pipe->set_custom_process_message([=](const char* buffer, size_t count) {
            string message(buffer, count);
            process_agent_message_(message, index + 1);
        });
    }
}

void spawner_new_c::setup_stream_(const options_class::redirect redirect, std_stream_type source_type, runner* this_runner) {
    auto source_pipe = this_runner->get_pipe(source_type);

    if (redirect.type == options_class::std) {
        if (source_type == std_stream_input) {
            get_std(std_stream_input, redirect.flags)->connect(source_pipe);
        }
        else {
            source_pipe->connect(get_std(source_type, redirect.flags));
        }
        return;
    }

    PANIC_IF(redirect.type != options_class::pipe);
    auto index = redirect.pipe_index;
    auto stream = redirect.name;
    PANIC_IF(index < 0 || index >= runners.size());

    auto target_runner = runners[index];

    multipipe_ptr target_pipe;
    if (stream == "stdin") {
        PANIC_IF(source_type == std_stream_input);
        target_pipe = target_runner->get_pipe(std_stream_input, redirect.flags);
        source_pipe->connect(target_pipe);
    }
    else if (stream == "stdout") {
        PANIC_IF(source_type != std_stream_input);
        target_pipe = target_runner->get_pipe(std_stream_output, redirect.flags);
        target_pipe->connect(source_pipe);
    }
    else if (stream == "stderr") {
        PANIC_IF(source_type != std_stream_input);
        target_pipe = target_runner->get_pipe(std_stream_error, redirect.flags);
        target_pipe->connect(source_pipe);
    }
    else {
        PANIC("invalid stream name");
    }

    if (control_mode_enabled) {
        if (source_type == std_stream_output) {
            setup_stream_in_control_mode_(this_runner, source_pipe);
        }
        else if (stream == "stdout") {
            setup_stream_in_control_mode_(target_runner, target_pipe);
        }
    }
}

bool spawner_new_c::init() {
    if (!init_runner() || !runners.size()) {
        return false;
    }
    for (size_t i = 0; i < runners.size(); i++) {
        if (runners[i]->get_options().controller) {
            // there must be only one controller process
            PANIC_IF(controller_index_ != -1);
            controller_index_ = i;
            control_mode_enabled = true;
            controller_broadcaster_ = runners[i]->get_pipe(std_stream_input);
        }
    }
    if (controller_index_ != -1) {
        awaited_agents_.resize(runners.size() - 1);
        for (size_t i = 0; i < runners.size(); i++) {
            secure_runner* sr = static_cast<secure_runner*>(runners[i]);
            if (i != controller_index_) {
                sr->start_suspended = true;
            }
            sr->on_terminate = [=]() {
                on_terminate_mutex_.lock();
                if (i > 0 && awaited_agents_[i - 1]) {
                    wait_agent_mutex_.lock();
                    awaited_agents_[i - 1] = false;
                    std::string message = std::to_string(i) + "I#\n";
                    //TODO: broadcast?
                    controller_broadcaster_->write(message.c_str(), message.size());
                    wait_agent_mutex_.unlock();
                }
                on_terminate_mutex_.unlock();
            };
        }
    }

    for (auto runner : runners) {
        options_class runner_options = runner->get_options();
        const struct {
            std::vector<options_class::redirect> &streams;
            std_stream_type type;
        } redirects_all[] = {
            { runner_options.stdinput, std_stream_input },
            { runner_options.stdoutput, std_stream_output },
            { runner_options.stderror, std_stream_error },
        };
        for (const auto& redirects : redirects_all) {
            for (const auto& redirect : redirects.streams) {
                PANIC_IF(redirect.original.size() == 0);
                if (redirect.type == options_class::file) {
                    continue;
                }
                setup_stream_(redirect, redirects.type, runner);
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
    options.session << order++ << time(nullptr) << runner::get_current_time();
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
        auto stdinput = secure_runner_instance->get_pipe(std_stream_input);
        auto stdoutput = secure_runner_instance->get_pipe(std_stream_output);
        auto stderror = secure_runner_instance->get_pipe(std_stream_error);

        for (auto& input : options.stdinput)
            if (input.type == options_class::file)
                get_or_create_file_pipe(input.name, read_mode, input.flags)->connect(stdinput);

        for (auto& output : options.stdoutput)
            if (output.type == options_class::file)
                stdoutput->connect(get_or_create_file_pipe(output.name, write_mode, output.flags));

        for (auto& error : options.stderror)
            if (error.type == options_class::file)
                stderror->connect(get_or_create_file_pipe(error.name, write_mode, error.flags));
    }

    secure_runner_instance->set_index(runners.size());
    runners.push_back(secure_runner_instance);
    return true;
}

void spawner_new_c::run() {
    begin_report();
    LOG("initialize...");
    for (auto i : runners) {
        i->run_process_async();
    }
    for (auto i : runners) {
        if (!i->wait_for_init(1000)) {
            PANIC("Failed to init process");
        }
    }
    for (const auto& file_pipe : file_pipes) {
        file_pipe.second->start_read();
    }
    for (auto i : runners) {
        i->get_pipe(std_stream_input)->check_parents();
    }
    LOG("initialized");
    for (auto i : runners) {
        i->wait_for();
    }
    for (auto i : runners) {
        i->finalize();
    }
    for (const auto& file_pipe : file_pipes) {
        file_pipe.second->finalize();
    }
    print_report();
}

static void maybe_write_to_file(const std::string &file_name, const std::string &report) {
    if (!file_name.length()) return;
    std::ofstream fo(file_name.c_str());
    fo << report;
}

void spawner_new_c::print_report() {
    if (runners.empty()) {
        std::string report = "Error: " + get_error_text();
        maybe_write_to_file(base_options.report_file, report);
        if (!base_options.hide_report)
            std::cout << report;
        return;
    }
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > report_writer(s);
    report_writer.StartArray();
    for (auto i : runners) {
        report_class rep = i->get_report();
        options_class options_item = i->get_options();
        std::cout.flush();
        if (!options_item.hide_report || options_item.report_file.length()) {
            std::string report;
            json_report(i, report_writer);

            if (options_item.login.length() > 0)
            {
                pull_shm_report(options_item.shared_memory.c_str(), report);
            }

            if (report.length() == 0)
            {
                if (!options_item.json)
                {
                    report = GenerateSpawnerReport(
                        rep, options_item,
                        i->get_restrictions()
                    );
                }
                else
                {
                    rapidjson::StringBuffer sub_report;
                    rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > report_item_writer(sub_report);
                    report_item_writer.StartArray();
                    json_report(i, report_item_writer);
                    report_item_writer.EndArray();
                    report = sub_report.GetString();
                }
            }

            if (options_item.delegated)
            {
                push_shm_report(options_item.shared_memory.c_str(), report);
            }
            else
            {
                if (!options_item.hide_report && runners.size() == 1)
                {
                    std::cout << report;
                }
                maybe_write_to_file(options_item.report_file, report);
            }
        }
    }
    report_writer.EndArray();
    if (base_options.json && runners.size() > 1) {
        std::string report = s.GetString();
        maybe_write_to_file(base_options.report_file, report);
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
        environment_default_parser->add_argument_parser(c_lst("SP_TIME_LIMIT"),
            new microsecond_argument_parser_c(restrictions[restriction_processor_time_limit]))
    )->set_description("Time limit (user process time)");
    console_default_parser->add_argument_parser(c_lst(short_arg("d")),
        environment_default_parser->add_argument_parser(c_lst("SP_DEADLINE"),
            new microsecond_argument_parser_c(restrictions[restriction_user_time_limit]))
            )->set_description("Time limit (wallclock time)");;
    console_default_parser->add_argument_parser(c_lst(short_arg("s")),
        environment_default_parser->add_argument_parser(c_lst("SP_SECURITY_LEVEL"),
            new bool_restriction_argument_parser_c(restrictions[restriction_security_limit]))
    );
    console_default_parser->add_argument_parser(c_lst(short_arg("ml")),
        environment_default_parser->add_argument_parser(c_lst("SP_MEMORY_LIMIT"),
            new byte_argument_parser_c(restrictions[restriction_memory_limit]))
    )->set_description("Memory limit");
    console_default_parser->add_argument_parser(c_lst(short_arg("wl")),
        environment_default_parser->add_argument_parser(c_lst("SP_WRITE_LIMIT"),
            new byte_argument_parser_c(restrictions[restriction_write_limit]))
    )->set_description("Output size limit");
    console_default_parser->add_argument_parser(c_lst(short_arg("lr")),
        environment_default_parser->add_argument_parser(c_lst("SP_LOAD_RATIO"),
            new percent_argument_parser_c(restrictions[restriction_load_ratio]))
    )->set_description("Declared idle processor load is below this (default: 5%)");
    console_default_parser->add_argument_parser(c_lst(short_arg("y")),
        environment_default_parser->add_argument_parser(c_lst("SP_IDLE_TIME_LIMIT"),
            new microsecond_argument_parser_c(restrictions[restriction_idle_time_limit]))
    )->set_description("Idleness time limit");

    console_default_parser->add_argument_parser(c_lst(short_arg("u")),
        environment_default_parser->add_argument_parser(c_lst("SP_USER"),
            new string_argument_parser_c(options.login))
    );
    console_default_parser->add_argument_parser(c_lst(short_arg("p")),
        environment_default_parser->add_argument_parser(c_lst("SP_PASSWORD"),
            new string_argument_parser_c(options.password))
    );

    console_default_parser->add_argument_parser(c_lst(short_arg("sr")),
        environment_default_parser->add_argument_parser(c_lst("SP_REPORT_FILE"),
            new string_argument_parser_c(options.report_file))
    )->set_description("Save report to file");

    console_default_parser->add_argument_parser(c_lst(short_arg("env")),
        environment_default_parser->add_argument_parser(c_lst("SP_ENVIRONMENT"),
            new environment_mode_argument_parser_c(options.environmentMode))
    )->set_description("Control environment variables for <executable> (default: inherit)");

    console_default_parser->add_argument_parser(c_lst(short_arg("D")),
        new options_callback_argument_parser_c(&options, &options_class::add_environment_variable)
    )->set_description("Define additional environment variable for <executable>");

    console_default_parser->add_argument_parser(c_lst(short_arg("ff"), long_arg("file-flags")),
        new options_callback_argument_parser_c(&options, &options_class::add_stdoutput)
    )->set_description("Define default file opening flags (f - force flush, e - exclusively open)");

    console_default_parser->add_argument_parser(c_lst(short_arg("so"), long_arg("out")),
        environment_default_parser->add_argument_parser(c_lst("SP_OUTPUT_FILE"),
            new options_callback_argument_parser_c(&options, &options_class::add_stdoutput))
    )->set_description("Redirect standard output stream to ([*[<file-flags>]:]<filename>|*[[<pipe-flags>]:]null|std|<index>.stdin)");
    console_default_parser->add_argument_parser(c_lst(short_arg("i"), long_arg("in")),
        environment_default_parser->add_argument_parser(c_lst("SP_INPUT_FILE"),
            new options_callback_argument_parser_c(&options, &options_class::add_stdinput))
    )->set_description("Redirect standard input stream from ([*[<file-flags>]:]<filename>|*[[<pipe-flags>]:]null|std|<index>.stdout)");
    console_default_parser->add_argument_parser(c_lst(short_arg("e"), short_arg("se"), long_arg("err")),
        environment_default_parser->add_argument_parser(c_lst("SP_ERROR_FILE"),
            new options_callback_argument_parser_c(&options, &options_class::add_stderror))
    )->set_description("Redirect standard error stream to ([*[<file-flags>]:]<filename>|*[[<pipe-flags>]:]null|std|<index>.stderr)");

    console_default_parser->add_argument_parser(c_lst(short_arg("runas"), long_arg("delegated")),
        environment_default_parser->add_argument_parser(c_lst("SP_RUNAS"), new boolean_argument_parser_c(options.delegated))
    );
    console_default_parser->add_argument_parser(c_lst(short_arg("ho")),
        environment_default_parser->add_argument_parser(c_lst("SP_HIDE_OUTPUT"), new boolean_argument_parser_c(options.hide_output))
    )->set_description("Do not display report on console");
    console_default_parser->add_argument_parser(c_lst(short_arg("hr")),
        environment_default_parser->add_argument_parser(c_lst("SP_HIDE_REPORT"), new boolean_argument_parser_c(options.hide_report))
    )->set_description("Do not display report on console");

    console_default_parser->add_argument_parser(c_lst(short_arg("sw")),
        environment_default_parser->add_argument_parser(c_lst("SP_SHOW_WINDOW"), new inverted_boolean_argument_parser_c(options.hide_gui))
    );

    console_default_parser->add_argument_parser(c_lst(long_arg("debug")),
        environment_default_parser->add_argument_parser(c_lst("SP_DEBUG"), new boolean_argument_parser_c(options.debug))
    );

    console_default_parser->add_argument_parser(c_lst(short_arg("mi"), long_arg("monitorInterval")),
        environment_default_parser->add_argument_parser(c_lst("SP_MONITOR_INTERVAL"),
            new microsecond_argument_parser_c(options.monitorInterval))
    )->set_description("Sleep interval for a monitoring thread (default: 0.001s)");

    console_default_parser->add_flag_parser(c_lst(short_arg("c"), long_arg("systempath")),
        environment_default_parser->add_argument_parser(c_lst("SP_SYSTEM_PATH"), new boolean_argument_parser_c(options.use_cmd))
    )->set_description("Search <executable> in system path");
    console_default_parser->add_argument_parser(c_lst(short_arg("wd")),
        environment_default_parser->add_argument_parser(c_lst("SP_DIRECTORY"), new string_argument_parser_c(options.working_directory))
    )->set_description("Set working directory");

    console_default_parser->add_flag_parser(c_lst(short_arg("j"), long_arg("json")),
        environment_default_parser->add_argument_parser(c_lst("SP_JSON"), new boolean_argument_parser_c(options.json))
    )->set_description("Use JSON for reporting");

    console_default_parser->add_argument_parser(c_lst(long_arg("separator")),
        environment_default_parser->add_argument_parser(c_lst("SP_SEPARATOR"),
            new callback_argument_parser_c<settings_parser_c*, void(settings_parser_c::*)(const std::string&)>(&parser, &settings_parser_c::set_separator))
    );

    console_default_parser->add_argument_parser(c_lst(short_arg("process-count")),
        new time_argument_parser_c<degree_default>(restrictions[restriction_processes_count_limit]));

    console_default_parser->add_argument_parser(c_lst(long_arg("shared-memory")),
        environment_default_parser->add_argument_parser(c_lst("SP_SHARED_MEMORY"), new string_argument_parser_c(options.shared_memory))
    );

    console_default_parser->add_flag_parser(c_lst(long_arg("controller")),
        new boolean_argument_parser_c(options.controller));

    console_default_parser->add_flag_parser(c_lst(SEPARATOR_ARGUMENT),
        new callback_argument_parser_c<spawner_new_c*, void(spawner_new_c::*)(const std::string&)>(&(*this), &spawner_new_c::on_separator));

    //ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(long_arg("program")), c_lst("SP_PROGRAM"),   options.session_id, STRING_CONVERT);

    parser.add_parser(console_default_parser);
    parser.add_parser(environment_default_parser);
}

std::string spawner_new_c::help() {
    string redirect_example =
        "\nRedirect examples:\n"
        "\tfile.txt      basic redirect to file\n"
        "\t*:file.txt    use default file flags\n"
        "\t*-f:std       disable flush on std stream. IMPORTANT! first std redirect hide all next flags\n"
        "\t*-f:0.stdin   disable flush on stdin redirect\n"
        "\t*f:file.txt   enable file flush\n"
        "\t*e:file.txt   open file exclusively\n"
        "\t*fe:file.txt  many flags\n"
        "\t*fe:          set defaults for files\n"
        "\t*:            reset defaults for files\n";
    return
        "Spawner: cross-platform sandboxing utility\n"
        "Usage: sp <options> <executable> <executable arguments>\n\n"
        "Options:\n" + parser.help() + redirect_example;
}
