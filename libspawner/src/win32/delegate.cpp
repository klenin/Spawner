#include <map>
#include <string>
#include <sstream>
#include <Windows.h>

#include "inc/delegate.h"
#include "inc/error.h"

const char *SPAWNER_PROGRAM = "sp.exe";

delegate_runner::delegate_runner(const std::string &program,
    const options_class &options, const restrictions_class &restrictions)
    : runner(SPAWNER_PROGRAM, options)
    , restrictions(restrictions)
    , program_to_run(program)
{

}

void delegate_runner::create_process() {
    char path[MAX_PATH + 1];

    if (GetFullPathNameA(program_to_run.c_str(), MAX_PATH, path, nullptr))
    {
        program_to_run = path;
    }

    options.push_argument_front(program_to_run);

    if (options.use_cmd)
    {
        options.push_argument_front("--cmd");
    }

    options.use_cmd = true;

    const std::map< restriction_kind_t, std::string > cmd_units = {
        { restriction_user_time_limit, "ms" },
        { restriction_memory_limit, "B" },
        { restriction_processor_time_limit, "us" },
        { restriction_security_limit, "" },
        { restriction_write_limit, "B" },
        { restriction_load_ratio, "" },
        { restriction_idle_time_limit, "us" },
        { restriction_processes_count_limit, "" }
    };

    const std::map< restriction_kind_t, std::string > cmd_arg = {
        { restriction_user_time_limit, "tl" },
        { restriction_memory_limit, "ml" },
        { restriction_processor_time_limit, "d" },
        { restriction_security_limit, "s" },
        { restriction_write_limit, "wl" },
        { restriction_load_ratio, "lr" },
        { restriction_idle_time_limit, "y" },
        { restriction_processes_count_limit, "only-process" }
    };

    for (int i = 0; i < restriction_max; ++i)
    {
        if (restrictions.restrictions[i] != restriction_no_limit)
        {
            std::string argument = "-" + cmd_arg.find((restriction_kind_t)i)->second;

            argument += " " + std::to_string(restrictions.restrictions[i]);
            argument += cmd_units.find((restriction_kind_t)i)->second;

            options.push_argument_front(argument);
        }
    }

    auto process_pipes = [](options_class& options, const std::vector<options_class::redirect>& vals, const std::string& prefix) {
        for (const auto& i : vals)
        {
            options.push_argument_front(prefix + i.original);
        }
    };

    process_pipes(options, options.stderror, "--err=");
    process_pipes(options, options.stdoutput, "--out=");
    process_pipes(options, options.stdinput, "--in=");

    std::string working_directory = options.working_directory;

    if (working_directory.length() == 0)
    {
        char dir[MAX_PATH + 1];

        GetCurrentDirectoryA(MAX_PATH, dir);

        working_directory = dir;
    }

    options.push_argument_front("-wd \"" + working_directory + "\"");

    if (options.hide_report)
    {
        options.push_argument_front("-hr 1");
    }

    for (const auto& i : options.environmentVars)
    {
        options.push_argument_front("-D " + i.first + "=" + i.second);
    }

    if (options.json)
    {
        options.push_argument_front("--json");
    }

    options.push_argument_front("-env " + options.environmentMode);

    std::string shared_memory_name = "mem" + options.session.hash();

    options.push_argument_front("--shared-memory=" + shared_memory_name);

    CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        options_class::SHARED_MEMORY_BUF_SIZE,
        shared_memory_name.c_str()
   );

    options.shared_memory = shared_memory_name;

    options.push_argument_front("--delegated:1");

    runner::create_process();
}
