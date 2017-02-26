#include "inc/delegate.hpp"

#include <map>
#include <string>
#include <sstream>

#include <sys/mman.h> //shm_{}
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h> // ftruncate
#include <sys/types.h>

#include "inc/error.hpp"

const char *SPAWNER_PROGRAM = "/usr/local/bin/sp";

delegate_runner::delegate_runner(const std::string &program,
    const options_class &options, const restrictions_class &restrictions)
    : runner(SPAWNER_PROGRAM, options)
    , restrictions(restrictions)
    , program_to_run(program)
{

}

void delegate_runner::create_process() {
    char *path = realpath(program_to_run.c_str(), NULL);
    if (path != nullptr) {
        program_to_run = path;
    free(path);
    } else
        PANIC("Failed to realpath();");

    options.push_argument_front(program_to_run);

    if (options.use_cmd) {
        PANIC("--cmd not supported here");
        options.push_argument_front("--cmd");
    }

    //options.use_cmd = true;

    const std::map< restriction_kind_t, std::string > cmd_units = {
        { restriction_user_time_limit, "us" },
        { restriction_memory_limit, "B" },
        { restriction_processor_time_limit, "us" },
        { restriction_security_limit, "" },
        { restriction_write_limit, "B" },
        { restriction_load_ratio, "" },
        { restriction_idle_time_limit, "us" },
        { restriction_processes_count_limit, "" }
    };

    const std::map< restriction_kind_t, std::string > cmd_arg = {
        { restriction_user_time_limit, "d" },
        { restriction_memory_limit, "ml" },
        { restriction_processor_time_limit, "tl" },
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

            argument += "=" + std::to_string(restrictions.restrictions[i]);
            argument += cmd_units.find((restriction_kind_t)i)->second;

            options.push_argument_front(argument);
        }
    }

    auto process_pipes = [](options_class& options, const std::vector<std::string>& vals, const std::string& prefix) {
        for (auto i = vals.cbegin(); i != vals.cend(); ++i)
        {
            options.push_argument_front(prefix + *i);
        }
    };

    process_pipes(options, options.stderror, "--err=");
    process_pipes(options, options.stdoutput, "--out=");
    process_pipes(options, options.stdinput, "--in=");

    std::string working_directory = options.working_directory;
    char *cwd;
    if (working_directory.length() == 0) {
    cwd = getcwd(NULL, 0);
    if (cwd != nullptr)
        working_directory = cwd;
    else
        PANIC("Failed to getcwd() for working directory");
    free(cwd);
    }

    options.push_argument_front("-wd=" + working_directory);

    if (options.hide_report)
    {
        options.push_argument_front("-hr=1");
    }

    for (auto i = options.environmentVars.cbegin(); i != options.environmentVars.cend(); ++i)
    {
        options.push_argument_front("-D " + i->first + "=" + i->second);
    }

    if (options.json)
    {
        options.push_argument_front("--json");
    }

    options.push_argument_front("-env=" + options.environmentMode);
    std::string shared_memory_name = "/" + options.session.hash();
    options.push_argument_front("--shared-memory=" + shared_memory_name);
    
    // XXX set umask to all-zero to allow creating a file with 0666 permissions
    // (defaults are to disallow o+w).
    // Access rights to shmem file shall be limited to group+rw (0660) in
    // future. I recommend to use a delegate runner with seccomp enabled to
    // limit access to teh shared memory.
    mode_t old = umask(0000);
    int shm_fd = shm_open(shared_memory_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
        PANIC("Failed to shm_open()");
    if (ftruncate(shm_fd, options_class::SHARED_MEMORY_BUF_SIZE) == -1) {
    shm_unlink(shared_memory_name.c_str());
        PANIC("Failed to ftruncate() shmem to specified size");
    }
    umask(old);
    options.shared_memory = shared_memory_name;
    options.push_argument_front("--delegated=1");
    
    runner::create_process();
}
