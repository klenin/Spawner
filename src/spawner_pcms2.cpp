#include "spawner_pcms2.h"

#include <iostream>

spawner_pcms2_c::spawner_pcms2_c(settings_parser_c &parser)
    : spawner_old_c(parser)
{

}

void spawner_pcms2_c::begin_report()
{
    if (!options.hide_report) {
        std::cout << "Running \"" << parser.get_program();
        if (options.get_arguments_count()) {
            std::cout << " " << options.get_arguments();
        }
        std::cout << "\", press ESC to terminate...\n";
    }
}

bool spawner_pcms2_c::init()
{
    options.hide_report = options.hide_output;
    return spawner_old_c::init();
}

void spawner_pcms2_c::print_report()
{
    report_class rep = runner_instance->get_report();
    options_class options = runner_instance->get_options();
    if (!options.hide_report) {
        switch (rep.terminate_reason) {
        case terminate_reason_memory_limit:
            std::cout << "Memory limit exceeded" << std::endl;
            std::cout << "Program tried to allocate more than " << restrictions[restriction_memory_limit] << " bytes" << std::endl;
            break;
        case terminate_reason_load_ratio_limit:
            std::cout << "Program utilized less than " << convert(value_t(unit_no_unit, degree_m4), value_t(unit_no_unit, degree_centi), (long double)restrictions[restriction_load_ratio]) << "% of processor time for more than "
              << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), (long double)restrictions[restriction_idle_time_limit]) << " sec" << std::endl;
            std::cout << "Idleness limit exceeded" << std::endl;
            break;
        case terminate_reason_time_limit:
            std::cout << "Time limit exceeded" << std::endl;
            std::cout << "Program failed to terminate within " <<
              convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), (long double)restrictions[restriction_processor_time_limit]) << " sec" << std::endl;
            break;
        default:
            std::cout << "Program successfully terminated" << std::endl;
            break;
        }
        std::cout << "  time consumed: 0.03";
        if (restrictions[restriction_processor_time_limit] != restriction_no_limit) {
            std::cout << " of " << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), (long double)restrictions[restriction_processor_time_limit]);
        }
        std::cout << " sec" << std::endl;
        std::cout << "  time passed:   " << rep.total_time << " sec" << std::endl;
        std::cout << "  peak memory:   " << rep.peak_memory_used;
        if (restrictions[restriction_memory_limit] != restriction_no_limit) {
            std::cout << " of " << restrictions[restriction_memory_limit];
        }
        std::cout << " bytes" << std::endl;

    }
    if (options.report_file.length()) {
        std::ofstream file(options.report_file);
        file << "average.memoryConsumed=" << rep.peak_memory_used << std::endl;
        file << "average.timeConsumed=" << rep.user_time << std::endl;
        file << "average.timePassed=" << rep.processor_time << std::endl;
        file << "invocations=1" << std::endl;
        file << "last.memoryConsumed=" << rep.peak_memory_used << std::endl;
        file << "last.timeConsumed=" << rep.user_time << std::endl;
        file << "last.timePassed=" << rep.processor_time << std::endl;
        file << "max.memoryConsumed=" << rep.peak_memory_used << std::endl;
        file << "max.timeConsumed=" << rep.user_time << std::endl;
        file << "max.timePassed=" << rep.processor_time << std::endl;
        file << "min.memoryConsumed=" << rep.peak_memory_used << std::endl;
        file << "min.timeConsumed=" << rep.user_time << std::endl;
        file << "min.timePassed=" << rep.processor_time << std::endl;
        file << "total.memoryConsumed=" << rep.peak_memory_used << std::endl;
        file << "total.timeConsumed=" << rep.user_time << std::endl;
        file << "total.timePassed=" << rep.processor_time << std::endl;
        file.close();
    }
}

std::string spawner_pcms2_c::help()
{
    return spawner_base_c::help() + "\
\n\
RUN for Windows NT, Version 3.0\n\
Copyright(c) SPb IFMO CTD Development Team, 2000-2010, Written by Andrew Stankevich\n\
\n\
This program runs other program for given period of time with specified\n\
  memory restrictions\n\
  \n\
Command line format:\n\
  run [<options>] <program> [<parameters>]\n\
Where options are:\n\
  -h               - show this help\n\
  -t <time-limit>  - time limit, terminate after <time-limit> seconds, you can\n\
                     add \"ms\" (without quotes) after the number to specify\n\
                     time limit in milliseconds\n\
  -m <mem-limit>   - memory limit, terminate if working set of the process\n\
                     exceeds <mem-limit> bytes, you can add K or M to specify\n\
                     memory limit in kilo- or megabytes\n\
  -r <req-load>    - required load of the processor for this process\n\
                     not to be considered idle. You can add % sign to specify\n\
                     required load in percent, default is 0.05 = 5%\n\
  -y <idle-limit>  - ildeness limit, terminate process if it did not load\n\
                     processor for at least <req-load> for <idleness-limit>\n\
  -d <directory>   - make <directory> home directory for process\n\
  -l <login-name>  - create process under <login-name>\n\
  -p <password>    - logins user using <password>\n\
  -i <file>        - redirects standart input stream to the <file>\n\
  -o <file>        - redirects standart output stream to the <file>\n\
  -e <file>        - redirects standart error stream to the <file>\n\
  -x               - return exit code of the application #not implemented yet\n\
  -q               - do not display any information on the screen\n\
  -w               - display program window on the screen\n\
  -1               - use single CPU/CPU core #not implemented yet\n\
  -s <file>        - store statistics in then <file>\n\
  -D var=value     - sets value of the environment variable, current environment\n\
                     is completly ignored in this case #not implemented yet\n\
Exteneded options:\n\
  -Xacp, --allow-create-processes #not implemented yet\n\
                   - allow the created process to create new processes\n\
  -Xtfce, --terminate-on-first-chance-exceptions #not implemented yet\n\
                   - do not ignore exceptions if they are marked as first-chance,\n\
                     required for some old compilers as Borland Delphi\n\
  -Xlegacy, -z #not implemented yet\n\
                   - try to be compatible with old invoke.dll\n\
Examples:\n\
  run -t 10s -m 32M -i 10s a.exe\n\
  run -d \"C:\\My Directory\" a.exe\n\
  run -l invoker -p password a.exe\n\
  run -i input.txt -o output.txt -e error.txt a.exe\n";
}

void spawner_pcms2_c::init_arguments()
{
    parser.set_dividers(c_lst("=").vector());
    options.hide_output = true;

    console_argument_parser_c *console_default_parser = new console_argument_parser_c();
    environment_variable_parser_c *environment_default_parser = new environment_variable_parser_c();

    console_default_parser->add_argument_parser(c_lst(short_arg("t")), new microsecond_argument_parser_c(restrictions[restriction_processor_time_limit]));
    console_default_parser->add_argument_parser(c_lst(short_arg("m")), new byte_argument_parser_c(restrictions[restriction_memory_limit]));
    console_default_parser->add_argument_parser(c_lst(short_arg("r")), new percent_argument_parser_c(restrictions[restriction_load_ratio]));
    console_default_parser->add_argument_parser(c_lst(short_arg("y")), new microsecond_argument_parser_c(restrictions[restriction_idle_time_limit]));

    console_default_parser->add_argument_parser(c_lst(short_arg("l")), new string_argument_parser_c(options.login));
    console_default_parser->add_argument_parser(c_lst(short_arg("d")), new string_argument_parser_c(options.working_directory));
    console_default_parser->add_argument_parser(c_lst(short_arg("p")), new string_argument_parser_c(options.password));
    console_default_parser->add_argument_parser(c_lst(short_arg("s")), new string_argument_parser_c(options.report_file));
    console_default_parser->add_argument_parser(c_lst(short_arg("o")), new string_argument_parser_c(output_file));
    console_default_parser->add_argument_parser(c_lst(short_arg("e")), new string_argument_parser_c(error_file));
    console_default_parser->add_argument_parser(c_lst(short_arg("i")), new string_argument_parser_c(input_file));

    console_default_parser->add_flag_parser(c_lst(short_arg("q")), new boolean_argument_parser_c(options.hide_output));
    console_default_parser->add_flag_parser(c_lst(short_arg("w")), new inverted_boolean_argument_parser_c(options.hide_gui));

    parser.add_parser(console_default_parser);
    parser.add_parser(environment_default_parser);
}

void spawner_pcms2_c::init_std_streams()
{

}
