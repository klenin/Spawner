#include "spawner_old.h"

#include <iostream>

spawner_old_c::spawner_old_c(settings_parser_c &parser)
    : parser(parser)
    , spawner_base_c()
    , options(session_class::base_session)
    , runas(false)
{

}

void spawner_old_c::init_std_streams()
{
    if (!options.hide_output) {
        options.add_stdoutput("std");
    }
}

spawner_old_c::~spawner_old_c()
{
    if (secure_runner_instance) {
        delete secure_runner_instance;
    }
}

bool spawner_old_c::init()
{
    if (!parser.get_program().length()) {
        return false;
    }
    if (options.login.length()) {
        options.delegated = true;
    }
    if (output_file.length()) {
        options.add_stdoutput(output_file);
    }
    if (error_file.length()) {
        options.add_stderror(error_file);
    }
    if (input_file.length()) {
        options.add_stdinput(input_file);
    }
    init_std_streams();
    options.add_arguments(parser.get_program_arguments());
    if (options.delegated) {
        secure_runner_instance = new delegate_runner(parser.get_program(), options, restrictions);
    }
    else if (options.session_id.length()){
        secure_runner_instance = new delegate_instance_runner(parser.get_program(), options, restrictions);
    }
    else {
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
    return true;
}

void spawner_old_c::run()
{
    begin_report();
    secure_runner_instance->run_process_async();
    secure_runner_instance->wait_for();

    print_report();
}

void spawner_old_c::print_report()
{
    report_class rep = secure_runner_instance->get_report();
    options_class options = secure_runner_instance->get_options();
    std::cout.flush();
    if (!options.hide_report || options.report_file.length()) {
        std::string report;
        report = GenerateSpawnerReport(
            rep, secure_runner_instance->get_options(),
            secure_runner_instance->get_restrictions()
            );
        if (!options.hide_report) {
            std::cout << report;
        }
        if (options.report_file.length())
        {
            std::ofstream fo(options.report_file.c_str());
            fo << report;
            fo.close();
        }
    }
}

std::string spawner_old_c::help()
{
    return spawner_base_c::help() + "\
Spawner options:\n\
\t Argument            Environment variable     Description\n\
\t-ml:[n]             SP_MEMORY_LIMIT     Iaeneiaeuiue iauai ae?ooaeuiie iaiyoe, auaaeaiiue i?ioanno (a Mb).\n\
\t-tl:[n]             SP_TIME_LIMIT       Iaeneiaeuiia a?aiy auiieiaiey i?ioanna a iieuciaaoaeuneii ?a?eia (a nae).\n\
\t-d:[n]              SP_DEADLINE         Eeieo oece?aneiai a?aiaie, auaaeaiiiai i?ioanno (a nae).\n\
\t-wl:[n]             SP_WRITE_LIMIT      Iaeneiaeuiue iauai aaiiuo, eioi?ue ii?ao auou caienai i?ioannii (a Mb).\n\
\t-u:[user@domain]    SP_USER             Eiy iieuciaaoaey a oi?iaoa: User[@Domain]\n\
\t-p:[password]       SP_PASSWORD         Ia?ieu.\n\
\t-runas:[0|1]        SP_RUNAS            Eniieuciaaou na?aen RunAs aey caionea i?ioanna.\n\
\t-s:[n]              SP_SECURITY_LEVEL   O?iaaiu aaciianiinoe. Ii?ao i?eieiaou cia?aiey 0 eee 1.\n\
\t-hr:[0|1]           SP_HIDE_REPORT      Ia iieacuaaou io?ao.\n\
\t-ho:[0|1]           SP_HIDE_OUTPUT      Ia iieacuaaou auoiaiie iioie (STDOUT) i?eei?aiey.\n\
\t-sr:[file]          SP_REPORT_FILE      Nio?aieou io?ao a oaee.\n\
\t-so:[file]          SP_OUTPUT_FILE      Nio?aieou auoiaiie iioie a oaee.\n\
\t-i:[file]           SP_INPUT_FILE       Iieo?eou aoiaiie iioie ec oaeea. \n";
}

void spawner_old_c::init_arguments()
{
    parser.set_dividers(c_lst(":").vector());

    console_argument_parser_c *console_default_parser = new console_argument_parser_c();
    environment_variable_parser_c *environment_default_parser = new environment_variable_parser_c();

    console_default_parser->add_argument_parser(c_lst(short_arg("tl")),
        environment_default_parser->add_argument_parser(c_lst("SP_TIME_LIMIT"), new microsecond_argument_parser_c(restrictions[restriction_processor_time_limit]))
        );
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
    console_default_parser->add_argument_parser(c_lst(short_arg("lr"), short_arg("r")),
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
    console_default_parser->add_argument_parser(c_lst(short_arg("so")),
        environment_default_parser->add_argument_parser(c_lst("SP_OUTPUT_FILE"), new string_argument_parser_c(output_file))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("i")),
        environment_default_parser->add_argument_parser(c_lst("SP_INPUT_FILE"), new string_argument_parser_c(input_file))
        );

    console_default_parser->add_argument_parser(c_lst(short_arg("runas")),
        environment_default_parser->add_argument_parser(c_lst("SP_RUNAS"), new boolean_argument_parser_c(runas))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("ho")),
        environment_default_parser->add_argument_parser(c_lst("SP_HIDE_OUTPUT"), new boolean_argument_parser_c(options.hide_output))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("sw")),
        environment_default_parser->add_argument_parser(c_lst("SP_SHOW_WINDOW"), new inverted_boolean_argument_parser_c(options.hide_gui))
        );
    console_default_parser->add_argument_parser(c_lst(short_arg("hr")),
        environment_default_parser->add_argument_parser(c_lst("SP_HIDE_REPORT"), new boolean_argument_parser_c(options.hide_report))
        );

    parser.add_parser(console_default_parser);
    parser.add_parser(environment_default_parser);
}
