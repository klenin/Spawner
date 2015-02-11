#include "arguments.h"
#include <iostream>
#include <fstream>
#include <string>

#include <sp.h>
#include <inc/compatibility.h>

class string_argument_parser_c : public base_argument_parser_c<std::string> {
public:
    string_argument_parser_c(std::string &value) : base_argument_parser_c<std::string>(value) {}
    virtual bool set(const std::string &s) {
        value = s;
        return true;
    }
};

template<typename O, typename C>
class callback_argument_parser_c : public string_argument_parser_c {
private:
    O o;
    C c;
    std::string v;
public:
    callback_argument_parser_c(O o, C c): o(o), c(c), string_argument_parser_c(v) {}
    virtual bool set(const std::string &s) {
        (o->*c)(s);
        return true;
    }
};

template<typename O, typename C>
class function_argument_parser_c : public string_argument_parser_c {
private:
    O o;
    C c;
    std::string v;
public:
    function_argument_parser_c(O o, C c): o(o), c(c), string_argument_parser_c(v) {}
    virtual bool set(const std::string &s) {
        if ((o->*c)(s)) {
            return true;
        }
        error = default_error;
        return false;
    }
};

typedef callback_argument_parser_c<options_class*, void(options_class::*)(const std::string&)> options_callback_argument_parser_c;

template<typename T, unit_t u, degrees_enum d>
class unit_argument_parser_c : public base_argument_parser_c<T> {
public:
    unit_argument_parser_c(T &value) : base_argument_parser_c<T>(value) {}
    virtual bool set(const std::string &s) {
        try {
            this->value = convert(value_t(u, d), s, restriction_no_limit);
        } catch(std::string &str) {
            this->error = "Value type is not compatible: " + str;
            return false;
        }
        return true;
    }
};

template<degrees_enum d>
class time_argument_parser_c : public unit_argument_parser_c<restriction_t, unit_time_second, d> {
protected:
    virtual bool accept_zero() {
        return false;
    }
public:
    time_argument_parser_c(restriction_t &value) : unit_argument_parser_c<restriction_t, unit_time_second, d>(value) {}
    virtual bool set(const std::string &s) {
        if (!unit_argument_parser_c<restriction_t, unit_time_second, d>::set(s)) {
            return false;
        }
        if (this->value == 0 && !accept_zero()) {
            this->error = "Time cannot be set to 0";
            return false;
        }
        return true;
    }
};

typedef time_argument_parser_c<degree_milli> millisecond_argument_parser_c;
typedef time_argument_parser_c<degree_micro> microsecond_argument_parser_c;

class byte_argument_parser_c : public unit_argument_parser_c<restriction_t, unit_memory_byte, degree_default> {
protected:
    virtual bool accept_zero() {
        return false;
    }
public:
    byte_argument_parser_c(restriction_t &value) : unit_argument_parser_c<restriction_t, unit_memory_byte, degree_default>(value) {}
    virtual bool set(const std::string &s) {
        if (!unit_argument_parser_c<restriction_t, unit_memory_byte, degree_default>::set(s)) {
            return false;
        }
        if (value == 0 && !accept_zero()) {
            error = "Memory cannot be set to 0";
            return false;
        }
        return true;
    }
};

class percent_argument_parser_c : public unit_argument_parser_c<restriction_t, unit_no_unit, degree_m4> {
protected:
    virtual bool accept_zero() {
        return false;
    }
public:
    percent_argument_parser_c(restriction_t &value) : unit_argument_parser_c<restriction_t, unit_no_unit, degree_m4>(value) {}
    virtual bool set(const std::string &s) {
        if (!unit_argument_parser_c<restriction_t, unit_no_unit, degree_m4>::set(s)) {
            return false;
        }
        if (value == 0 && !accept_zero()) {
            error = "Ratio cannot be set to 0";
            return false;
        }
        return true;
    }
};

template<typename T, bool inverted = false>
class base_boolean_argument_parser_c : public base_argument_parser_c<T> {
protected:
    virtual T true_value() {
        return !inverted;
    }
    virtual T false_value() {
        return inverted;
    }
public:
    base_boolean_argument_parser_c(T &value) : base_argument_parser_c<T>(value) {}
    virtual bool set(const std::string &s) {
        if (s == "1") {
            this->value = true_value();
        } else if (s == "0") {
            this->value = false_value();
        } else {
            this->error = "Value type is not compatible";
            return false;
        }
        return true;
    }
};

typedef base_boolean_argument_parser_c<bool> boolean_argument_parser_c;
typedef base_boolean_argument_parser_c<bool, true> inverted_boolean_argument_parser_c;

class bool_restriction_argument_parser_c : public base_boolean_argument_parser_c<restriction_t> {
public:
    bool_restriction_argument_parser_c(restriction_t &value) : base_boolean_argument_parser_c<restriction_t>(value) {}
protected:
    virtual restriction_t true_value() {
        return restriction_limited;
    }
    virtual restriction_t false_value() {
        return restriction_no_limit;
    }
};


class function_restriction_argument_parser_c : public abstract_argument_parser_c {
};


class spawner_base_c {
protected:
    std::vector<runner*> runners;
    std::map<std::string, runner*> runner_alias;
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
    virtual void begin_report() {}
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






class spawner_old_c: public spawner_base_c {
protected:
    restrictions_class restrictions;
    options_class options;
    bool runas;
    settings_parser_c &parser;
    std::string report_file;
    std::string output_file;
    std::string error_file;
    std::string input_file;
    secure_runner *secure_runner_instance;
    //std::string program;

public:
    spawner_old_c(settings_parser_c &parser): parser(parser), spawner_base_c(), options(session_class::base_session), runas(false) {
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
    virtual void run() {
        begin_report();
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
                fo << report;
                fo.close();
            }
        }
    }
    virtual std::string help() {
        return spawner_base_c::help() + "\
Spawner options:\n\
\t Argument            Environment variable     Description\n\
\t-ml:[n]             SP_MEMORY_LIMIT     Ìàêñèìàëüíûé îáúåì âèðòóàëüíîé ïàìÿòè, âûäåëåííûé ïðîöåññó (â Mb).\n\
\t-tl:[n]             SP_TIME_LIMIT       Ìàêñèìàëüíîå âðåìÿ âûïîëíåíèÿ ïðîöåññà â ïîëüçîâàòåëüñêîì ðåæèìå (â ñåê).\n\
\t-d:[n]              SP_DEADLINE         Ëèìèò ôèçè÷åñêîãî âðåìåíè, âûäåëåííîãî ïðîöåññó (â ñåê).\n\
\t-wl:[n]             SP_WRITE_LIMIT      Ìàêñèìàëüíûé îáúåì äàííûõ, êîòîðûé ìîæåò áûòü çàïèñàí ïðîöåññîì (â Mb).\n\
\t-u:[user@domain]    SP_USER             Èìÿ ïîëüçîâàòåëÿ â ôîðìàòå: User[@Domain]\n\
\t-p:[password]       SP_PASSWORD         Ïàðîëü.\n\
\t-runas:[0|1]        SP_RUNAS            Èñïîëüçîâàòü ñåðâèñ RunAs äëÿ çàïóñêà ïðîöåññà.\n\
\t-s:[n]              SP_SECURITY_LEVEL   Óðîâåíü áåçîïàñíîñòè. Ìîæåò ïðèíèìàòü çíà÷åíèÿ 0 èëè 1.\n\
\t-hr:[0|1]           SP_HIDE_REPORT      Íå ïîêàçûâàòü îò÷åò.\n\
\t-ho:[0|1]           SP_HIDE_OUTPUT      Íå ïîêàçûâàòü âûõîäíîé ïîòîê (STDOUT) ïðèëîæåíèÿ.\n\
\t-sr:[file]          SP_REPORT_FILE      Ñîõðàíèòü îò÷åò â ôàéë.\n\
\t-so:[file]          SP_OUTPUT_FILE      Ñîõðàíèòü âûõîäíîé ïîòîê â ôàéë.\n\
\t-i:[file]           SP_INPUT_FILE       Ïîëó÷èòü âõîäíîé ïîòîê èç ôàéëà. \n";
    }
    virtual void init_arguments() {
    
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
};

class spawner_pcms2_c: public spawner_old_c {
public:
    spawner_pcms2_c(settings_parser_c &parser): spawner_old_c(parser) {
    }
    virtual void begin_report() {
        if (!options.hide_report) {
            std::cout << "Running \"" << parser.get_program();
            if (options.get_arguments_count()) {
                std::cout << " " << options.get_arguments();
            }
            std::cout << "\", press ESC to terminate...\n";
        }
    }
    virtual bool init() {
        options.hide_report = options.hide_output;
        return spawner_old_c::init();
    }
    virtual void print_report() {
        report_class rep = secure_runner_instance->get_report();
        options_class options = secure_runner_instance->get_options();
        if (!options.hide_report) {
            switch (rep.terminate_reason) {
            case terminate_reason_memory_limit:
                std::cout << "Memory limit exceeded" << std::endl;
                std::cout << "Program tried to allocate more than " << restrictions[restriction_memory_limit] << " bytes" << std::endl;
                break;
            case terminate_reason_load_ratio_limit:
                std::cout << "Program utilized less than " << convert(value_t(unit_no_unit, degree_m4), value_t(unit_no_unit, degree_centi), (long double)restrictions[restriction_load_ratio]) <<"% of processor time for more than " 
                    << convert(value_t(unit_time_second, degree_milli), value_t(unit_time_second), (long double)restrictions[restriction_idle_time_limit]) <<" sec" << std::endl;
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
    virtual void init_arguments() {
    
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
    virtual void init_std_streams() {
    }
};

class spawner_new_c: public spawner_base_c {
protected:
    restrictions_class restrictions;
    options_class options;
    bool runas;
    settings_parser_c &parser;
    std::vector<secure_runner*> runners;
    size_t order;
public:
    spawner_new_c(settings_parser_c &parser): parser(parser), spawner_base_c(), options(session_class::base_session), runas(false), order(0) {
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

    Json::Value json_report(runner *runner_instance) {
//        Json::Value report(Json::arrayValue);
        //for {
        report_class runner_report = runner_instance->get_report();
        options_class runner_options = runner_instance->get_options();
        Json::Value report_item(Json::objectValue);

        report_item["Application"] = runner_report.application_name;
        report_item["Arguments"] = Json::Value(Json::arrayValue);
        for (size_t i = 0; i < runner_options.get_arguments_count(); ++i) {
            report_item["Arguments"].append(runner_options.get_argument(i));
        }

        Json::Value limit(Json::objectValue);
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
            if (restriction_items[i].degree == degree_default) {
                limit[restriction_items[i].field] = runner_restrictions[restriction_items[i].restriction];
            } else {
                limit[restriction_items[i].field] = (double)convert(
                    value_t(restriction_items[i].unit, restriction_items[i].degree),
                    value_t(restriction_items[i].unit),
                    (long double)runner_restrictions[restriction_items[i].restriction]
                );
            }
        }
        
        Json::Value option(Json::objectValue);
        struct {
            char *field;
            Json::Value value;
            unit_t unit;
            degrees_enum degree;
        } option_items[] = {
            { "SearchInPath", runner_options.use_cmd, unit_no_unit, degree_default },
            { NULL, 0, unit_no_unit, degree_default },
        };
        for (int i = 0; option_items[i].field; ++i) {
            if (option_items[i].degree == degree_default) {
                option[option_items[i].field] = option_items[i].value;
            } else {
                option[option_items[i].field] = (double)convert(
                    value_t(option_items[i].unit, option_items[i].degree),
                    value_t(option_items[i].unit),
                    (long double)option_items[i].value.asDouble()
                );
            }
        }
        Json::Value result(Json::objectValue);
        struct {
            char *field;
            Json::Value value;
            unit_t unit;
            degrees_enum degree;
        } result_items[] = {
            { "Time", runner_report.processor_time, unit_time_second, degree_micro },
            { "WallClockTime", runner_report.user_time, unit_time_second, degree_micro },
            { "Memory", (Json::Value::UInt64)runner_report.peak_memory_used, unit_memory_byte, degree_default },
            { "BytesWritten", runner_report.write_transfer_count, unit_memory_byte, degree_default },
            { "KernelTime", runner_report.kernel_time, unit_time_second, degree_micro },
            { "ProcessorLoad", runner_report.load_ratio, unit_no_unit, degree_centi },
            { "WorkingDirectory", runner_report.working_directory, unit_no_unit, degree_default },
            { NULL, 0, unit_no_unit, degree_default },
        };
        for (int i = 0; result_items[i].field; ++i) {
            if (result_items[i].degree == degree_default) {
                result[result_items[i].field] = result_items[i].value;
            } else {
                result[result_items[i].field] = (double)convert(
                    value_t(result_items[i].unit, result_items[i].degree),
                    value_t(result_items[i].unit),
                    (long double)result_items[i].value.asDouble()
                );
            }
        }

        Json::Value std_out(Json::arrayValue);
        for (uint i = 0; i < runner_options.stdoutput.size(); ++i) {
            std_out.append(runner_options.stdoutput[i]);
        }

        report_item["Limit"] = limit;
        report_item["Result"] = result;
        report_item["Options"] = option;
        report_item["StdOut"] = std_out;
        report_item["CreateProcessMethod"]   = (options.login==""?"CreateProcess":"WithLogon");
        report_item["UserName"]              = runner_report.login;

        report_item["TerminateReason"]       = get_terminate_reason(runner_report.terminate_reason);
	    report_item["ExitCode"]              = runner_report.exit_code;
	    report_item["ExitStatus"]            = ExitCodeToString(runner_report.exit_code);
        report_item["SpawnerError"]          = Json::Value(Json::arrayValue);
        while (error_list::remains()) {
            report_item["SpawnerError"].append(error_list::pop_error());
        }
        return report_item;
    }
    virtual bool init() {
        if (!init_runner()) {
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
                    } else if (stream == "stdin") {
                        pipe_type = STD_INPUT_PIPE;
                    } else if (stream == "stdout") {
                        pipe_type = STD_OUTPUT_PIPE;
                    } else {
                        return false;
                    }
                    secure_runner *target_runner = runners[index];
                    duplex_buffer_class *buffer = new duplex_buffer_class();
                    if (stream_item.pipe_type == STD_INPUT_PIPE && pipe_type != STD_INPUT_PIPE) {
                        static_cast<input_pipe_class*>((*i)->get_pipe(stream_item.pipe_type))->add_input_buffer(buffer);
                        static_cast<output_pipe_class*>(target_runner->get_pipe(pipe_type))->add_output_buffer(buffer);
                    } else if (stream_item.pipe_type != STD_INPUT_PIPE && pipe_type == STD_INPUT_PIPE) {
                        static_cast<input_pipe_class*>(target_runner->get_pipe(pipe_type))->add_input_buffer(buffer);
                        static_cast<output_pipe_class*>((*i)->get_pipe(stream_item.pipe_type))->add_output_buffer(buffer);
                    } else {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    bool init_runner() {
        if (!parser.get_program().length()) {
            return false;
            //throw exception
        }
        secure_runner *secure_runner_instance;
        options.session << order++ << time(NULL);
        options.add_arguments(parser.get_program_arguments());
        if (options.login.length()) {
            secure_runner_instance = new delegate_runner(parser.get_program(), options, restrictions);
        } else if (options.session_id.length()){
            secure_runner_instance = new delegate_instance_runner(parser.get_program(), options, restrictions);
        } else {
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
    virtual void run() {
        begin_report();
        for (auto i = runners.begin(); i != runners.end(); i++) {
            (*i)->run_process_async();
        }
        for (auto i = runners.begin(); i != runners.end(); i++) {
            (*i)->wait_for();
        }
        print_report();
    }
    virtual void print_report() {
        Json::Value report_object(Json::arrayValue);
        for (auto i = runners.begin(); i != runners.end(); i++) {
            report_class rep = (*i)->get_report();
            options_class options = (*i)->get_options();
            std::cout.flush();
            Json::Value report_item = report_object.append(json_report(*i));
            if (!options.hide_report || options.report_file.length()) {
                std::string report;
			    if (!options.json) {
                    report = GenerateSpawnerReport(
                        rep, (*i)->get_options(), 
                        (*i)->get_restrictions()
                    );
                } else {
                    report = report_item.toStyledString();
                }
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
        std::cout << report_object.toStyledString();
    }
    virtual std::string help() {
        return parser.help();/*spawner_base_c::help() + "\
Spawner options:\n\
\t Argument            Environment variable     Description\n\
\t-ml:[n]             SP_MEMORY_LIMIT     Ìàêñèìàëüíûé îáúåì âèðòóàëüíîé ïàìÿòè, âûäåëåííûé ïðîöåññó (â Mb).\n\
\t-tl:[n]             SP_TIME_LIMIT       Ìàêñèìàëüíîå âðåìÿ âûïîëíåíèÿ ïðîöåññà â ïîëüçîâàòåëüñêîì ðåæèìå (â ñåê).\n\
\t-d:[n]              SP_DEADLINE         Ëèìèò ôèçè÷åñêîãî âðåìåíè, âûäåëåííîãî ïðîöåññó (â ñåê).\n\
\t-wl:[n]             SP_WRITE_LIMIT      Ìàêñèìàëüíûé îáúåì äàííûõ, êîòîðûé ìîæåò áûòü çàïèñàí ïðîöåññîì (â Mb).\n\
\t-u:[user@domain]    SP_USER             Èìÿ ïîëüçîâàòåëÿ â ôîðìàòå: User[@Domain]\n\
\t-p:[password]       SP_PASSWORD         Ïàðîëü.\n\
\t-runas:[0|1]        SP_RUNAS            Èñïîëüçîâàòü ñåðâèñ RunAs äëÿ çàïóñêà ïðîöåññà.\n\
\t-s:[n]              SP_SECURITY_LEVEL   Óðîâåíü áåçîïàñíîñòè. Ìîæåò ïðèíèìàòü çíà÷åíèÿ 0 èëè 1.\n\
\t-hr:[0|1]           SP_HIDE_REPORT      Íå ïîêàçûâàòü îò÷åò.\n\
\t-ho:[0|1]           SP_HIDE_OUTPUT      Íå ïîêàçûâàòü âûõîäíîé ïîòîê (STDOUT) ïðèëîæåíèÿ.\n\
\t-sr:[file]          SP_REPORT_FILE      Ñîõðàíèòü îò÷åò â ôàéë.\n\
\t-so:[file]          SP_OUTPUT_FILE      Ñîõðàíèòü âûõîäíîé ïîòîê â ôàéë.\n\
\t-i:[file]           SP_INPUT_FILE       Ïîëó÷èòü âõîäíîé ïîòîê èç ôàéëà. \n";*/
    }
    virtual void on_separator(const std::string &_) {
        init_runner();
        parser.clear_program_parser();
        options = options_class(session_class::base_session);
        restrictions = restrictions_class();
    }
    virtual void init_arguments() {
    
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
        console_default_parser->add_flag_parser(c_lst(long_arg("cmd"), long_arg("systempath")), 
            environment_default_parser->add_argument_parser(c_lst("SP_SYSTEM_PATH"), new boolean_argument_parser_c(options.use_cmd))
        );
        console_default_parser->add_argument_parser(c_lst(short_arg("wd")), 
            environment_default_parser->add_argument_parser(c_lst("SP_DIRECTORY"), new string_argument_parser_c(options.working_directory))
        );

        console_default_parser->add_flag_parser(c_lst(long_arg("json")), 
            environment_default_parser->add_argument_parser(c_lst("SP_JSON"), new boolean_argument_parser_c(options.json))
        );

        console_default_parser->add_argument_parser(c_lst(long_arg("session")), new string_argument_parser_c(options.session_id));

        console_default_parser->add_argument_parser(c_lst(long_arg("separator")), 
            environment_default_parser->add_argument_parser(c_lst("SP_SEPARATOR"), new callback_argument_parser_c<settings_parser_c*, void(settings_parser_c::*)(const std::string&)>(&parser, &settings_parser_c::set_separator))
        );



        console_default_parser->add_flag_parser(c_lst(SEPARATOR_ARGUMENT), new callback_argument_parser_c<spawner_new_c*, void(spawner_new_c::*)(const std::string&)>(&(*this), &spawner_new_c::on_separator));

        //ADD_CONSOLE_ENVIRONMENT_ARGUMENT(old_spawner, c_lst(long_arg("program")), c_lst("SP_PROGRAM"),   options.session_id, STRING_CONVERT);

        parser.add_parser(console_default_parser);
        parser.add_parser(environment_default_parser);

    }
};


void command_handler_c::add_default_parser() {
    console_argument_parser_c *console_default_parser = new console_argument_parser_c();
    environment_variable_parser_c *environment_default_parser = new environment_variable_parser_c();

    console_default_parser->add_flag_parser(c_lst(short_arg("h"), long_arg("help")), new boolean_argument_parser_c(show_help))
        ->set_description("Show help");
    console_default_parser->add_argument_parser(
        c_lst(long_arg("legacy")), 
        new function_argument_parser_c<command_handler_c*, spawner_base_c*(command_handler_c::*)(const std::string&)>(
            this, 
            &command_handler_c::create_spawner
        )
    )->set_description("Spawner interface")->default_error = environment_default_parser->add_argument_parser(
        c_lst("SP_LEGACY"), 
        new function_argument_parser_c<command_handler_c*, spawner_base_c*(command_handler_c::*)(const std::string&)>(
            this, 
            &command_handler_c::create_spawner
        )
    )->default_error = "Invalid value for legacy argument.";

    add_parser(console_default_parser);
    add_parser(environment_default_parser);
}
command_handler_c::command_handler_c(): spawner(NULL), show_help(false), legacy_set(false) {
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
    return create_spawner(s) != NULL;
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
        spawner = new spawner_new_c(this->parser);
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
    if (!p->invoke_initialization(parser)) {
        parser.stop();
    }
}

bool command_handler_c::parse(int argc, char *argv[]) {
//    reset();
    if (!parser.parse(argc, argv)) {
        return false;
    }
    if (show_help) {
        std::cout << spawner->help();
        return true;
    }
    if (spawner && spawner->init()) {
        spawner->run();
    } else {
        std::cout << spawner->help();
    }
    return true;
}


command_handler_c handler;


BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType) {
    //handler.spawner->stop();
    handler.spawner->print_report();
    return FALSE;
}


int main(int argc, char *argv[]) {
    SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
    if (!handler.parse(argc, argv)) {

    }
    return 0;
}
