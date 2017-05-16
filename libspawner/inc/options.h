#ifndef _SPAWNER_OPTIONS_H_
#define _SPAWNER_OPTIONS_H_

#include <list>
#include <vector>

#include "inc/error.h"
#include "inc/session.h"

// hide_gui - creates process with hidden flag turned on
// silent_errors - on windows, if error occurs program doesn't show "send report" dialog

// TODO: make some options as map of variant
// TODO: rewrite as a class

struct options_class
{
    static const unsigned SHARED_MEMORY_BUF_SIZE = 4069;

    enum redirect_type {
        file,
        pipe,
        std,
    };

    struct redirect_flags {
        bool flush;
        bool exclusive;
        void apply(const redirect_flags &src);
    };

    static const redirect_flags file_default;
    static const redirect_flags pipe_default;

    struct redirect {
        redirect_type type = file;
        std::string name = "";
        std::string original = "";
        int pipe_index = -1;
        redirect_flags flags = file_default;
    };

    session_class session;
    std::string report_file;  /* bad, need many values
                                 but this causes many outputs if environment
                                 variable and command line argument both present */
    std::string shared_memory;

    std::string string_arguments;
    std::string working_directory;

    std::string login;
    std::string password;

    redirect_flags global_in_file_flags;
    redirect_flags global_out_file_flags;
    redirect_flags global_err_file_flags;
    std::vector<redirect> stdinput;
    std::vector<redirect> stdoutput;
    std::vector<redirect> stderror;
    std::list<std::string> arguments;

    std::list< std::pair< std::string, std::string > > environmentVars;
    std::string environmentMode = "inherit";

    bool hide_gui = true;
    bool hide_report = false;
    bool hide_output = false;
    bool debug = false;
    bool json = false;
    bool secure_token = false;
    bool silent_errors = true;
    bool use_cmd = false;
    bool delegated = false;
    bool controller = false;
    unsigned long monitorInterval = 1000; // 0.001s

    explicit options_class(const session_class &session_param)
        : session(session_param)
        , global_in_file_flags(file_default)
        , global_out_file_flags(file_default)
        , global_err_file_flags(file_default) {}

    void add_argument(std::string argument);
    void add_arguments(const std::vector<std::string> &arguments_a);
    void push_argument_front(std::string argument);
    void parse_flags(std::string flags_string, const redirect_flags &global_flags, const redirect_flags &default_flags, redirect_flags &dst_flags) const;
    bool parse_redirect(std::string redirect_string, redirect_flags &global_flags, redirect &redirect_result) const;
    void add_std(std::vector<redirect> &dst, redirect_flags &global_flags, const std::string &redirect_str) const;
    void add_stdinput(const std::string &redirect_str);
    void add_stdoutput(const std::string &redirect_str);
    void add_stderror(const std::string &redirect_str);
    void add_environment_variable(const std::string &envStr);
    void clear_stdinput();
    void clear_stdoutput();
    void clear_stderror();
    std::string get_arguments() const;
    std::string get_argument(size_t index) const;
    size_t get_arguments_count() const;
    std::string format_arguments() const;
};

#endif //_SPAWNER_OPTIONS_H_

