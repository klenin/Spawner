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
{

}

spawner_new_c::~spawner_new_c()
{
    for (auto i = runners.begin(); i != runners.end(); i++) {
        delete (*i);
    }
}

void spawner_new_c::json_report(runner *runner_instance,
    rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF16<> > &writer)
{
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
    std::vector<std::string> errors = error_list::get_errors();
    for (int i = 0; i < errors.size(); i++) {
        rapidjson_write(errors[i].c_str());
    }
    writer.EndArray();
    writer.EndObject();
}

bool spawner_new_c::init()
{
    if (!init_runner() || !runners.size()) {
        return false;
    }
    for (auto i = runners.begin(); i != runners.end(); i++) {
        options_class runner_options = (*i)->get_options();
        struct {
            std::vector<std::string> &streams;
            pipes_t pipe_type;
        } streams[] = {
            { runner_options.stdinput, STD_INPUT_PIPE },
            { runner_options.stdoutput, STD_OUTPUT_PIPE },
            { runner_options.stderror, STD_ERROR_PIPE },
        };
        for (int k = 0; k < 3; ++k) {
            auto stream_item = streams[k];
            for (auto j = stream_item.streams.begin(); j != stream_item.streams.end(); j++) {
                if ((*j)[0] != '*') {
                    continue;
                }
                size_t pos = (*j).find(".");
                if (pos == std::string::npos) {
                    //error
                    return false;
                }
                size_t index = stoi((*j).substr(1, pos - 1), nullptr, 10);
                std::string stream = (*j).substr(pos + 1);
                if (index > runners.size()) {
                    //error
                    return false;
                }
                pipes_t pipe_type;
                if (stream == "stderr") {
                    pipe_type = STD_ERROR_PIPE;
                }
                else if (stream == "stdin") {
                    pipe_type = STD_INPUT_PIPE;
                }
                else if (stream == "stdout") {
                    pipe_type = STD_OUTPUT_PIPE;
                }
                else {
                    return false;
                }
                runner *target_runner = runners[index];
                duplex_buffer_class *buffer = new duplex_buffer_class();
                if (stream_item.pipe_type == STD_INPUT_PIPE && pipe_type != STD_INPUT_PIPE) {
                    static_cast<input_pipe_class*>((*i)->get_pipe(stream_item.pipe_type))->add_input_buffer(buffer);
                    static_cast<output_pipe_class*>(target_runner->get_pipe(pipe_type))->add_output_buffer(buffer);
                }
                else if (stream_item.pipe_type != STD_INPUT_PIPE && pipe_type == STD_INPUT_PIPE) {
                    static_cast<input_pipe_class*>(target_runner->get_pipe(pipe_type))->add_input_buffer(buffer);
                    static_cast<output_pipe_class*>((*i)->get_pipe(stream_item.pipe_type))->add_output_buffer(buffer);
                }
                else {
                    return false;
                }
            }
        }
    }
    return true;
}

bool spawner_new_c::init_runner()
{
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
    runners.push_back(secure_runner_instance);
    return true;
}

void spawner_new_c::run()
{
    begin_report();
    for (auto i = runners.begin(); i != runners.end(); i++) {
        (*i)->run_process_async();
    }
    for (auto i = runners.begin(); i != runners.end(); i++) {
        (*i)->wait_for();
    }
    print_report();
}

void spawner_new_c::print_report()
{
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

void spawner_new_c::on_separator(const std::string &_)
{
    init_runner();
    parser.clear_program_parser();
    options = options_class(base_options);
    restrictions = restrictions_class(base_restrictions);
}

void spawner_new_c::init_arguments()
{
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
        environment_default_parser->add_argument_parser(c_lst("SP_RUNAS"), new boolean_argument_parser_c(runas))
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

    console_default_parser->add_flag_parser(c_lst(SEPARATOR_ARGUMENT), new callback_argument_parser_c<spawner_new_c*, void(spawner_new_c::*)(const std::string&)>(&(*this), &spawner_new_c::on_separator));

    //ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(long_arg("program")), c_lst("SP_PROGRAM"),   options.session_id, STRING_CONVERT);

    parser.add_parser(console_default_parser);
    parser.add_parser(environment_default_parser);
}

std::string spawner_new_c::help()
{
    return parser.help();
}
