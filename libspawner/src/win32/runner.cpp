#include <inc/runner.h>
#include <inc/error.h>

#include <iostream>
const size_t MAX_USER_NAME = 1024;


bool runner::init_process(char *cmd, const char *wd)
{
    std::string run_program = program;
    if (force_program.length())
        run_program = force_program;
    if ( !CreateProcess(run_program.c_str(),
        cmd,
        NULL, NULL,
        TRUE,
        process_creation_flags,
        NULL, wd,
        &si, &process_info) )
    {
        if (!options.use_cmd || !CreateProcess(NULL,
                cmd,
                NULL, NULL,
                TRUE,
                process_creation_flags,
                NULL, wd,
                &si, &process_info) )
            {
                raise_error(*this, "CreateProcess");
                return false;
            }
    }
    return true;
}

bool runner::init_process_with_logon(char *cmd, const char *wd)
{
    STARTUPINFOW siw;
    //USES_CONVERSION;
    ZeroMemory(&siw, sizeof(siw));
    siw.cb = sizeof(si);
    siw.dwFlags = si.dwFlags;
    //siw.dwFlags = STARTF_USESTDHANDLES;
    siw.hStdInput = si.hStdInput;
    siw.hStdOutput = si.hStdOutput;
    siw.hStdError = si.hStdError;
    siw.wShowWindow = si.wShowWindow;
    siw.lpDesktop = L"";
    std::string run_program = program;
    if (force_program.length())
        run_program = force_program;

    wchar_t *login = a2w(options.login.c_str());
    wchar_t *password = a2w(options.password.c_str());
    wchar_t *wprogram = a2w(run_program.c_str());
    wchar_t *wcmd = a2w(cmd);
    wchar_t *wwd = a2w(wd);

    DWORD creation_flags = CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW;
    //wchar_t *login = a2w(options.login.c_str());
    if ( !CreateProcessWithLogonW(login, NULL, password, 0,
        wprogram, wcmd, creation_flags,
        NULL, wwd, &siw, &process_info) )
    {
        if (!options.use_cmd || !CreateProcessWithLogonW(login, NULL, password, 0,
            NULL, wcmd, creation_flags,
            NULL, wwd, &siw, &process_info) )
        {
            raise_error(*this, "CreateProcessWithLogonW");
            delete[] login;
            delete[] password;
            delete[] wprogram;
            delete[] wcmd;
            delete[] wwd;
            return false;
        }
    }
    delete[] login;
    delete[] password;
    delete[] wprogram;
    delete[] wcmd;
    delete[] wwd;
    return true;
}

void runner::create_process()
{
    WaitForSingleObject(init_mutex, INFINITE);

    if (process_status == process_spawner_crash)
        return;
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    if (!options.delegated) {//#TODO fix this
        si.dwFlags = STARTF_USESTDHANDLES;
        if (pipes.find(STD_OUTPUT_PIPE) != pipes.end())
            si.hStdOutput = pipes[STD_OUTPUT_PIPE]->get_pipe();
        if (pipes.find(STD_ERROR_PIPE) != pipes.end())
            si.hStdError = pipes[STD_ERROR_PIPE]->get_pipe();
        if (pipes.find(STD_INPUT_PIPE) != pipes.end())
            si.hStdInput = pipes[STD_INPUT_PIPE]->get_pipe();
    }
    si.lpDesktop = "";
    process_creation_flags = PROCESS_CREATION_FLAGS;
 
    if (options.hide_gui)
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    if (options.silent_errors)
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (options.debug)
        process_creation_flags |= DEBUG_PROCESS;

    // Extracting program name and generating cmd line
    char *cmd;
    report.working_directory = options.working_directory;
    const char *wd = (options.working_directory != "")?options.working_directory.c_str():NULL;
    if (!wd)
    {
        char working_directory[MAX_PATH + 1];
        if (GetCurrentDirectoryA(MAX_PATH, working_directory))//error here is not critical
            report.working_directory = working_directory;
    }
    std::string run_program = program;
    if (force_program.length())
        run_program = force_program;

    std::string command_line;
    size_t  index_win = run_program.find_last_of('\\'),
        index_nix = run_program.find_last_of('/');

    if (index_win != std::string::npos) {
        command_line = run_program.substr(index_win + 1);
    } else if (index_nix != std::string::npos) {
        command_line = run_program.substr(index_nix + 1);
    } else {
        command_line = run_program;
    }

    command_line = command_line + " ";
    if (options.string_arguments == "") {
        command_line += options.get_arguments();
    } else {
        command_line += options.string_arguments;
    }
    cmd = new char [command_line.size()+1];
    strcpy(cmd, command_line.c_str());
    if (options.login != "" && init_process_with_logon(cmd, wd))
    {
        report.login = options.login;
        ReleaseMutex(init_mutex);
        delete[] cmd;
        return;
    }
    //IMPORTANT: if logon option selected & failed signalize it
    DWORD len = MAX_USER_NAME;
    char user_name[MAX_USER_NAME];

    if (GetUserNameA(user_name, &len))//error here is not critical
        report.login = user_name;

    //std::cout << cmd;
    running = init_process(cmd, wd);

    ReleaseMutex(init_mutex);
    delete[] cmd;
}

void runner::free()
{
    for (std::map<pipes_t, pipe_class*>::iterator it = pipes.begin(); it != pipes.end(); ++it) {
        delete it->second;
    }
    CloseHandleSafe(process_info.hProcess);
    CloseHandleSafe(process_info.hThread);
}

void runner::wait()
{
    WaitForSingleObject(process_info.hProcess, INFINITE);
}

void runner::debug() {
    DEBUG_EVENT debug_event;
    while (WaitForDebugEvent(&debug_event, INFINITE))
    {
        ContinueDebugEvent(debug_event.dwProcessId,
            debug_event.dwThreadId,
            DBG_CONTINUE);
        if (debug_event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            debug_event.dwProcessId = 0;
        }
        //std::cout << debug_event.u.DebugString.lpDebugStringData << std::endl;
    }
}

void runner::requisites() {
    for (std::map<pipes_t, pipe_class*>::iterator it = pipes.begin(); it != pipes.end(); ++it) {
        pipe_class *pipe = it->second;
        if (!pipe->bufferize()) {
            raise_error(*this, LAST);
            return;
        }
    }
    if (ResumeThread(process_info.hThread) == (DWORD)-1)
    {
        raise_error(*this, "ResumeThread");
        return;
    }
    if (options.debug) {
        debug();
    }
}

thread_return_t runner::async_body(thread_param_t param) {
    runner *self = (runner*)param;
    self->run_process();
    return 0;
}

runner::runner(const std::string &program, const options_class &options):
    program(program), options(options), process_status(process_not_started), running_async(false),
    running_thread(handle_default_value), running(false), init_mutex(handle_default_value) {
    init_mutex = CreateMutex(NULL, FALSE, NULL);
}

runner::~runner()
{
    free();
}

unsigned long runner::get_exit_code()
{
    if (process_status == process_spawner_crash)
        return 0;
    DWORD dwExitCode = 0;
    if (!GetExitCodeProcess(process_info.hProcess, &dwExitCode))
        raise_error(*this, "GetExitCodeProcess");
    return dwExitCode;
}

process_status_t runner::get_process_status()
{
    //renew process status
    if (process_status & process_finished_normal || process_status == process_suspended)
        return process_status;
    unsigned long exitcode = get_exit_code();
    if (process_status == process_spawner_crash)
        return process_status;
    if (exitcode == exit_code_still_active)
        process_status = process_still_active;
    else
        process_status = process_finished_abnormally;
    if (exitcode == 0)
        process_status = process_finished_normal;
    return process_status;
}

exception_t runner::get_exception()
{
    if (get_process_status() == process_finished_abnormally)
        return (exception_t)get_exit_code();
    else return exception_exception_no;
}

unsigned long runner::get_id()
{
    return process_info.dwProcessId;
}

std::string runner::get_program() const
{
    return program;
}

options_class runner::get_options() const
{
    return options;
}

report_class runner::get_report()
{
    report.application_name = get_program();

    report.exception = get_exception();
    report.exit_code = get_exit_code();
    return report;
}

void runner::run_process()
{
    if (options.debug && !running_async) {
        run_process_async();
        WaitForSingleObject(running_thread, 100);//may stuck here
        WaitForSingleObject(init_mutex, INFINITE);//may stuck here
        WaitForSingleObject(process_info.hProcess, INFINITE);//may stuck here
        return;
    }
    create_process();
    if (get_process_status() == process_spawner_crash || get_process_status() & process_finished_normal)
        return;
    running = true;
    requisites();
    wait();
}

void runner::run_process_async() {
    running_async = true;
    running_thread = CreateThread(NULL, 0, async_body, this, 0, NULL);
}

bool runner::wait_for(const unsigned long &interval)
{
    if (get_process_status() == process_spawner_crash || get_process_status() & process_finished_normal)
        return true;
    return WaitForSingleObject(process_info.hProcess, interval) == WAIT_OBJECT_0;// TODO: get rid of this
    //std_output.wait_for_pipe(100);
    //std_error.wait_for_pipe(100);
    //std_input.wait_for_pipe(100);
}

bool runner::wait_for_init(const unsigned long &interval) {
    while (init_mutex == handle_default_value) {//not very good, made for synchro with async(mutex belongs to creator thread)
        Sleep(5);
    }
    return WaitForSingleObject(init_mutex, interval) == WAIT_OBJECT_0;// TODO: get rid of this
}

void runner::safe_release()
{
    process_status = process_spawner_crash;
    free();// make it safe!!!
}

void runner::set_pipe(const pipes_t &pipe_type, pipe_class *pipe_object)
{
    pipes[pipe_type] = pipe_object;
}