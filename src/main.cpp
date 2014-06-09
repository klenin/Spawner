#include <iostream>
#include <fstream>
#include <string>

#include <spawner.h>
#include <functional>

#include "arguments.h"

#define NEW_CONSOLE_PARSER(PARSER) console_argument_parser_c *console_parser_##PARSER = new console_argument_parser_c
#define NEW_ENVIRONMENT_PARSER(PARSER) environment_variable_parser_c *environment_parser_##PARSER = new environment_variable_parser_c
#define MILLISECOND_CONVERT convert<unit_time_second, degree_milli>
#define BYTE_CONVERT convert<unit_memory_byte, degree_default>
#define PERCENT_CONVERT convert<unit_no_unit, degree_m4>
#define BOOL_CONVERT convert_bool
#define STRING_CONVERT
#define ADD_CONSOLE_ARGUMENT(PARSER, ARGUMENTS, VALUE, TYPE_CONVERTER, ...) (console_parser_##PARSER->\
    add_parameter((std::vector<std::string>)ARGUMENTS, [this](std::string &s) -> bool {VALUE=TYPE_CONVERTER(s); __VA_ARGS__ ; return 1;}))
#define ADD_FLAG_ARGUMENT(PARSER, ARGUMENTS, VALUE, TYPE_CONVERTER, ...) do {\
console_parser_##PARSER->add_parameter((std::vector<std::string>)ARGUMENTS, [this](std::string &s) -> bool {VALUE=TYPE_CONVERTER(s); __VA_ARGS__ ; return 1;});\
console_parser_##PARSER->set_flag((std::vector<std::string>)ARGUMENTS);\
} while (0)
#define ADD_ENVIRONMENT_ARGUMENT(PARSER, ARGUMENTS, VALUE, TYPE_CONVERTER, ...) (environment_parser_##PARSER->\
    add_parameter((std::vector<std::string>)ARGUMENTS, [this](std::string &s) -> bool {VALUE=TYPE_CONVERTER(s); __VA_ARGS__ ; return 1;}))

#define ADD_CONSOLE_ENVIRONMENT_ARGUMENT(PARSER, CONSOLE_ARGUMENTS, ENVIRONMENT_ARGUMENTS, VALUE, TYPE_CONVERTER, ...) do {\
    ADD_CONSOLE_ARGUMENT(PARSER, CONSOLE_ARGUMENTS, VALUE, TYPE_CONVERTER, __VA_ARGS__);\
    ADD_ENVIRONMENT_ARGUMENT(PARSER, ENVIRONMENT_ARGUMENTS, VALUE, TYPE_CONVERTER, __VA_ARGS__);\
    } while (0)


class spawner_base_c {
protected:
    output_buffer_class *create_output_buffer(const std::string &name, const pipes_t &pipe_type, const size_t buffer_size = 4096) {
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
    input_buffer_class *create_input_buffer(const std::string &name, const size_t buffer_size = 4096) {
        input_buffer_class *input_buffer = NULL;
        if (name == "std") {
            input_buffer = new input_stdin_buffer_class(4096);
        } else if (name[0] == '*') {
        } else if (name.length()) {
            input_buffer = new input_file_buffer_class(name, 4096);
        }
        return input_buffer;
    }
public:
    spawner_base_c(){}
    virtual void run(int argc, char *argv[]){}
    virtual std::string help() {
        return "Usage:\n\t--legacy=<sp99|sp00|pcms2>\n";
    }
    virtual void init_arguments(){}
    virtual bool init() {return true;}
    virtual void print_report() {}
    virtual void run() {}
};


class command_handler_c {
protected:
    settings_parser_c parser;
    void add_default_parser();
    bool legacy_set;
public:
    bool show_help;
    spawner_base_c *spawner;
    command_handler_c();
    void reset();
    bool parse(int argc, char *argv[]);
    spawner_base_c *create_spawner(const std::string &s);
    bool set_legacy(const std::string &s);
    void add_parser(abstract_parser_c *p);
};






template<unit_t u, degrees_enum d>
restriction_t convert(const std::string &str) {
    return convert(value_t(u, d), str, restriction_no_limit);
}

restriction_t convert_bool(const std::string &str) {
    return str=="1"?restriction_limited:restriction_no_limit;
}

class spawner_old_c: public spawner_base_c {
protected:
    restrictions_class restrictions;
    options_class options;
    bool hide_report;
    bool hide_output;
    bool runas;
    settings_parser_c &parser;
    std::string report_file;
    std::string output_file;
    std::string error_file;
    std::string input_file;
    secure_runner *secure_runner_instance;
    //std::string program;

public:
    spawner_old_c(settings_parser_c &parser): parser(parser), spawner_base_c(), options(session_class::base_session), hide_report(false), hide_output(false), runas(false) {
    }
    virtual void init_std_streams() {
        if (!options.hide_output) {
            options.add_stdoutput("std");
        }
    }
    virtual bool init() {
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
        } else if (options.session_id.length()){
            secure_runner_instance = new delegate_instance_runner(parser.get_program(), options, restrictions);
        } else {
            secure_runner_instance = new secure_runner(parser.get_program(), options, restrictions);
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
        return true;
    }
    virtual void run() {

        secure_runner_instance->run_process_async();
        secure_runner_instance->wait_for();

        print_report();
    }
    virtual void print_report() {
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
                fo << GenerateSpawnerReport(rep, options, ((secure_runner*)secure_runner_instance)->get_restrictions());
                fo.close();
            }
        }
    }
    virtual std::string help() {
        return spawner_base_c::help() + "\
Spawner options:\n\
\t Argument            Environment variable     Description\n\
\t-ml:[n]             SP_MEMORY_LIMIT     Максимальный объем виртуальной памяти, выделенный процессу (в Mb).\n\
\t-tl:[n]             SP_TIME_LIMIT       Максимальное время выполнения процесса в пользовательском режиме (в сек).\n\
\t-d:[n]              SP_DEADLINE         Лимит физического времени, выделенного процессу (в сек).\n\
\t-wl:[n]             SP_WRITE_LIMIT      Максимальный объем данных, который может быть записан процессом (в Mb).\n\
\t-u:[user@domain]    SP_USER             Имя пользователя в формате: User[@Domain]\n\
\t-p:[password]       SP_PASSWORD         Пароль.\n\
\t-runas:[0|1]        SP_RUNAS            Использовать сервис RunAs для запуска процесса.\n\
\t-s:[n]              SP_SECURITY_LEVEL   Уровень безопасности. Может принимать значения 0 или 1.\n\
\t-hr:[0|1]           SP_HIDE_REPORT      Не показывать отчет.\n\
\t-ho:[0|1]           SP_HIDE_OUTPUT      Не показывать выходной поток (STDOUT) приложения.\n\
\t-sr:[file]          SP_REPORT_FILE      Сохранить отчет в файл.\n\
\t-so:[file]          SP_OUTPUT_FILE      Сохранить выходной поток в файл.\n\
\t-i:[file]           SP_INPUT_FILE       Получить входной поток из файла. \n";
    }
    virtual void init_arguments() {
    
        parser.set_dividers(c_lst(":").vector());

        NEW_CONSOLE_PARSER(old_spawner);
        NEW_ENVIRONMENT_PARSER(old_spawner);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("tl")),   c_lst("SP_TIME_LIMIT"),     restrictions[restriction_user_time_limit],  MILLISECOND_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("ml")),   c_lst("SP_MEMORY_LIMIT"),   restrictions[restriction_memory_limit],     BYTE_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("s")),    c_lst("SP_SECURITY_LEVEL"), restrictions[restriction_security_limit],   BOOL_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("d")),    c_lst("SP_DEADLINE"),       restrictions[restriction_processor_time_limit], MILLISECOND_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("wl")),   c_lst("SP_WRITE_LIMIT"),    restrictions[restriction_write_limit],      BYTE_CONVERT);


        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("u")),    c_lst("SP_USER"),           options.login,    STRING_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("p")),    c_lst("SP_PASSWORD"),       options.password, STRING_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("sr")),   c_lst("SP_REPORT_FILE"),    options.report_file,      STRING_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("so")),   c_lst("SP_OUTPUT_FILE"),    output_file,      STRING_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("i")),    c_lst("SP_INPUT_FILE"),     input_file,       STRING_CONVERT);

        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("runas")), c_lst("SP_RUNAS"),         runas,        BOOL_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("ho")),    c_lst("SP_HIDE_OUTPUT"),   options.hide_output,  BOOL_CONVERT);
        ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(short_arg("hr")),    c_lst("SP_HIDE_REPORT"),   options.hide_report,  BOOL_CONVERT);

        parser.add_parser(console_parser_old_spawner);
        parser.add_parser(environment_parser_old_spawner);
    }
};

class spawner_pcms2_c: public spawner_old_c {
public:
    spawner_pcms2_c(settings_parser_c &parser): spawner_old_c(parser) {
    }
    virtual std::string help() {
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
  -w               - display program window on the screen #not implemented yet\n\
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
    virtual void init_arguments() {
    
        parser.set_dividers(c_lst().vector());
        hide_output = true;

        NEW_CONSOLE_PARSER(old_spawner);
        NEW_ENVIRONMENT_PARSER(old_spawner);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("t")),    restrictions[restriction_user_time_limit],  MILLISECOND_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("m")),    restrictions[restriction_memory_limit],     BYTE_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("r")),    restrictions[restriction_load_ratio],       PERCENT_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("y")),    restrictions[restriction_idle_time_limit],  MILLISECOND_CONVERT);

        
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("l")),    options.login,    STRING_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("d")),    options.working_directory,    STRING_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("p")),    options.password, STRING_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("s")),    options.report_file,      STRING_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("o")),    output_file,      STRING_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("e")),    error_file,      STRING_CONVERT);
        ADD_CONSOLE_ARGUMENT(old_spawner, c_lst(short_arg("i")),    input_file,       STRING_CONVERT);

        ADD_FLAG_ARGUMENT(old_spawner, c_lst(short_arg("q")),    hide_report=hide_output,  BOOL_CONVERT);

        parser.add_parser(console_parser_old_spawner);
        parser.add_parser(environment_parser_old_spawner);
    }
    virtual void init_std_streams() {
    }
};

typedef std::function<void(std::string)> callback_t;

template<typename T, typename T1>
callback_t param_binder() {
    //return
}

void command_handler_c::add_default_parser() {
    NEW_CONSOLE_PARSER(default_parser);
    NEW_ENVIRONMENT_PARSER(default_parser);

    ADD_CONSOLE_ENVIRONMENT_ARGUMENT(default_parser, c_lst(long_arg("legacy")), c_lst("SP_LEGACY"), bool tmp, set_legacy, if (!tmp) return 0);
    ADD_FLAG_ARGUMENT(default_parser, c_lst(short_arg("h"), long_arg("help")), show_help, 1; , std::cout << spawner->help(); parser.stop());
    add_parser(console_parser_default_parser);
    add_parser(environment_parser_default_parser);
}
command_handler_c::command_handler_c(): spawner(NULL), show_help(false), legacy_set(false) {
//    reset();
    add_default_parser();
    if (!spawner) {
        create_spawner("sp00");
    }
}
bool command_handler_c::set_legacy(const std::string &s) {
    if (legacy_set) {
        return false;
    }
    legacy_set = true;
    return create_spawner(s);
}
void command_handler_c::reset() {
    while (parser.parsers_count() > 2) {
        parser.pop_back();
    }
    parser.set_dividers(c_lst("=").vector());
}
spawner_base_c *command_handler_c::create_spawner(const std::string &s) {
    reset();
    if (spawner) {
        delete spawner;
        spawner = NULL;
    }
    if (s == "sp99") {
        spawner = new spawner_old_c(this->parser);
    } else if (s == "sp00") {
        spawner = new spawner_base_c();
    } else if (s == "pcms2") {
        spawner = new spawner_pcms2_c(this->parser);
    }

    if (spawner) {
        spawner->init_arguments();
    }

    return spawner;
}
void command_handler_c::add_parser(abstract_parser_c *p) {
    parser.add_parser(p);
    p->invoke_initialization(parser);
}

bool command_handler_c::parse(int argc, char *argv[]) {
//    reset();
    parser.parse(argc, argv);

    if (spawner && spawner->init()) {
        spawner->run();
    } else {
    }
    return true;
}


int main(int argc, char *argv[]) {
    //spawner_base_c *spawner = new spawner_old_c();
    //spawner->run(argc, argv);
    command_handler_c handler;
    handler.parse(argc, argv);




    return 0;//*/
    /*
    spawner_c spawner(argc, argv);


    spawner.init();
    spawner.run();
    
	return 0;//*/
}
