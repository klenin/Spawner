#include <inc/runner.h>
#include <inc/error.h>

#ifdef _MSC_VER
#pragma comment(lib, "Userenv")
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <WinBase.h>
#include <UserEnv.h>

const size_t MAX_USER_NAME = 1024;

handle_t runner::main_job_object = handle_default_value;
handle_t runner::main_job_object_access_mutex = CreateMutex(NULL, 0, NULL);
bool runner::allow_breakaway = true;

void runner::set_allow_breakaway(bool allow) {
    if (allow_breakaway == allow) {
        return;
    }
    if (main_job_object == handle_default_value) {
        main_job_object = CreateJobObject(NULL, NULL);
        AssignProcessToJobObject(main_job_object, GetCurrentProcess());
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information;
    memset(&extended_limit_information, 0, sizeof(extended_limit_information));
    if (allow) {
        extended_limit_information.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    }

    if (!SetInformationJobObject(main_job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information))) {
        DWORD le = GetLastError();
        return;
    }
    allow_breakaway = allow;
}

void runner::copy_environment(TCHAR* dest, const WCHAR* source) const {
    int written = 0;

    for (WCHAR* env = (WCHAR*)source; *env != '\0';)
    {
        TCHAR* ansi = w2a((const WCHAR*)env);

        strcpy(dest, ansi);

        int bytes = strlen(ansi) + 1;

        written += bytes;
        env += bytes;
        dest += bytes;
    }
    
    *dest = '\0';
}

runner::env_vars_list_t runner::read_environment(const WCHAR* source) const
{
    env_vars_list_t vars;

    for (WCHAR* env = (WCHAR*)source; *env != '\0';)
    {
        std::string envStr(w2a((const WCHAR*)env));

        int pos = envStr.find("=");

        vars.push_back(make_pair(envStr.substr(0, pos), envStr.substr(pos + 1)));

        env += envStr.length() + 1;
    }

    return vars;
}

runner::env_vars_list_t runner::set_environment_for_process() const
{
    auto curr_vars = read_environment(GetEnvironmentStringsW());

    if (options.environmentMode == "user-default")
    {
        LPVOID envBlock = NULL;

        CreateEnvironmentBlock(&envBlock, NULL, FALSE);

        auto default_vars = read_environment((WCHAR*)envBlock);

        DestroyEnvironmentBlock(envBlock);

        for (auto i = default_vars.cbegin(); i != default_vars.cend(); ++i)
        {
            SetEnvironmentVariableA(i->first.c_str(), i->second.c_str());
        }

        for (auto i = curr_vars.cbegin(); i != curr_vars.cend(); ++i)
        {
            if (find(default_vars.cbegin(), default_vars.cend(), *i) == default_vars.cend())
            {
                SetEnvironmentVariableA(i->first.c_str(), NULL);
            }
        }
    }
    else if (options.environmentMode == "clear")
    {
        for (auto i = curr_vars.cbegin(); i != curr_vars.cend(); ++i)
        {
            SetEnvironmentVariableA(i->first.c_str(), NULL);
        }
    }

    for (auto i = options.environmentVars.cbegin(); i != options.environmentVars.cend(); ++i) {
        SetEnvironmentVariableA(i->first.c_str(), i->second.c_str());
    }

    return curr_vars;
}

void runner::restore_original_environment(const runner::env_vars_list_t& original) const
{
    auto curr_vars = read_environment(GetEnvironmentStringsW());

    for (auto i = original.cbegin(); i != original.cend(); ++i)
    {
        SetEnvironmentVariableA(i->first.c_str(), i->second.c_str());
    }

    for (auto i = curr_vars.cbegin(); i != curr_vars.cend(); ++i)
    {
        if (find(original.cbegin(), original.cend(), *i) == original.cend())
        {
            SetEnvironmentVariableA(i->first.c_str(), NULL);
        }
    }
}

bool runner::init_process(char *cmd, const char *wd) {
    WaitForSingleObject(main_job_object_access_mutex, infinite);
    set_allow_breakaway(true);

   // LPVOID penv = createEnvironmentForProcess();
    env_vars_list_t original = set_environment_for_process();

    std::string run_program = program;
    if (force_program.length())
        run_program = force_program;
    if ( !CreateProcess(run_program.c_str(),
            cmd, NULL, NULL,
            TRUE,
            process_creation_flags,
            NULL, wd,
            &si, &process_info) ) {
        if (!options.use_cmd || !CreateProcess(NULL,
                cmd,
                NULL, NULL,
                TRUE,
                process_creation_flags,
                NULL, wd,
                &si, &process_info) ) {
            ReleaseMutex(main_job_object_access_mutex);
            restore_original_environment(original);
            raise_error(*this, "CreateProcess");
            return false;
        }
    }
    ReleaseMutex(main_job_object_access_mutex);
    restore_original_environment(original);

    get_times(&creation_time, NULL, NULL, NULL);
    return true;
}

bool runner::init_process_with_logon(char *cmd, const char *wd) {
    WaitForSingleObject(main_job_object_access_mutex, infinite);
    set_allow_breakaway(false);

    STARTUPINFOW siw;
    //USES_CONVERSION;
    ZeroMemory(&siw, sizeof(siw));
    siw.cb = sizeof(si);
    siw.dwFlags = si.dwFlags;
    siw.hStdInput = si.hStdInput;
    siw.hStdOutput = si.hStdOutput;
    siw.hStdError = si.hStdError;
    siw.wShowWindow = si.wShowWindow;
    siw.lpDesktop = NULL;//L"";
    std::string run_program = program;
    if (force_program.length())
        run_program = force_program;

    wchar_t *login = a2w(options.login.c_str());
    wchar_t *password = a2w(options.password.c_str());
    wchar_t *wprogram = a2w(run_program.c_str());
    wchar_t *wcmd = a2w(cmd);
    wchar_t *wwd = a2w(wd);

    DWORD creation_flags = CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW;

    HANDLE token = NULL;

    auto original = set_environment_for_process();
    
    if ( !CreateProcessWithLogonW(login, NULL, password, 0,
        wprogram, wcmd, creation_flags,
        NULL, wwd, &siw, &process_info) )
    {
        if (!options.use_cmd || !CreateProcessWithLogonW(login, NULL, password, 0,
            NULL, wcmd, creation_flags,
            NULL, wwd, &siw, &process_info) )
        {
            ReleaseMutex(main_job_object_access_mutex);
            raise_error(*this, "CreateProcessWithLogonW");
            delete[] login;
            delete[] password;
            delete[] wprogram;
            delete[] wcmd;
            delete[] wwd;
            restore_original_environment(original);
            
            return false;
        }
    }

    set_allow_breakaway(true);
    ReleaseMutex(main_job_object_access_mutex);
    delete[] login;
    delete[] password;
    delete[] wprogram;
    delete[] wcmd;
    delete[] wwd;
    restore_original_environment(original);

    get_times(&creation_time, NULL, NULL, NULL);

    return true;
}

void runner::create_process() {
    //WaitForSingleObject(init_semaphore, INFINITE);

    if (process_status == process_spawner_crash) {
        ReleaseSemaphore(init_semaphore, 10, NULL);
        return;
    }
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    {//if (!options.delegated) {//#TODO fix this
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
    if (options.silent_errors) {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    }
    if (options.debug) {
        process_creation_flags |= DEBUG_PROCESS;
    }

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

    bool withLogon = options.login != "";

    if (withLogon) {
        report.login = a2w(options.login.c_str());
    } else {
        //IMPORTANT: if logon option selected & failed signalize it
        DWORD len = MAX_USER_NAME;
        wchar_t user_name[MAX_USER_NAME];
        if (GetUserNameW(user_name, &len)) {//error here is not critical
            report.login = user_name;
        }
    }

    running = withLogon ? init_process_with_logon(cmd, wd) : init_process(cmd, wd);

    ReleaseSemaphore(init_semaphore, 10, NULL);

    delete[] cmd;
}

void runner::free() {
    for (std::map<pipes_t, pipe_class*>::iterator it = pipes.begin(); it != pipes.end(); ++it) {
        delete it->second;
    }
    CloseHandleSafe(process_info.hProcess);
    CloseHandleSafe(process_info.hThread);
}

void runner::wait() {
    WaitForSingleObject(process_info.hProcess, INFINITE);
}

void runner::debug() {
    DEBUG_EVENT debug_event;
    while (WaitForDebugEvent(&debug_event, INFINITE)) {
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
    if (ResumeThread(process_info.hThread) == (DWORD)-1) {
        raise_error(*this, "ResumeThread");
        return;
    }
    for (std::map<pipes_t, pipe_class*>::iterator it = pipes.begin(); it != pipes.end(); ++it) {
        pipe_class *pipe = it->second;
        if (!pipe->bufferize()) {
            raise_error(*this, LAST);
            return;
        }
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
    running_thread(handle_default_value), running(false), init_semaphore(handle_default_value) {

    init_semaphore = CreateSemaphore(NULL, 0, 10, NULL);
    ZeroMemory(&process_info, sizeof(process_info));
}

runner::~runner() {
    free();
}

unsigned long runner::get_exit_code() {
    if (process_status == process_spawner_crash) {
        return 0;
    }
    DWORD dwExitCode = 0;
    if (!GetExitCodeProcess(process_info.hProcess, &dwExitCode)) {
        raise_error(*this, "GetExitCodeProcess");
    }
    return dwExitCode;
}

process_status_t runner::get_process_status() {
    //renew process status
    if (process_status & process_finished_normal || process_status == process_suspended || process_status == process_not_started) {
        return process_status;
    }
    unsigned long exitcode = get_exit_code();
    if (process_status == process_spawner_crash) {
        return process_status;
    }
    if (exitcode == exit_code_still_active) {
        process_status = process_still_active;
    } else {
        process_status = process_finished_abnormally;
    }
    if (exitcode == 0) {
        process_status = process_finished_normal;
    }
    return process_status;
}

exception_t runner::get_exception() {
    if (get_process_status() == process_finished_abnormally) {
        return (exception_t)get_exit_code();
    }
    else return exception_exception_no;
}

unsigned long runner::get_id() {
    return process_info.dwProcessId;
}

std::string runner::get_program() const {
    return program;
}

options_class runner::get_options() const {
    return options;
}

report_class runner::get_report() {
    report.application_name = get_program();

    report.exception = get_exception();
    report.exit_code = get_exit_code();
    return report;
}

unsigned long long runner::get_time_since_create() {
    unsigned long long current = runner::get_current_time();
    if (current < creation_time) {
        current = creation_time;
    }
    return current - creation_time;
}

unsigned long long runner::get_current_time() {
    SYSTEMTIME current_time_sys;
    FILETIME current_time;
    GetSystemTime(&current_time_sys);
    SystemTimeToFileTime(&current_time_sys, &current_time);

    return *reinterpret_cast<unsigned long long*>(&current_time);
}

handle_t runner::get_process_handle() {
    return process_info.hProcess;
}
void runner::get_times(unsigned long long *_creation_time, unsigned long long *exit_time, unsigned long long *kernel_time, unsigned long long *user_time) {
    FILETIME __creation_time;
    FILETIME _exit_time;
    FILETIME _kernel_time;
    FILETIME _user_time;

    GetProcessTimes(get_process_handle(), &__creation_time, &_exit_time, &_kernel_time, &_user_time);
    if (_creation_time) {
        *_creation_time = *reinterpret_cast<unsigned long long*>(&__creation_time);
    }
    if (exit_time) {
        *exit_time = *reinterpret_cast<unsigned long long*>(&_exit_time);
    }
    if (kernel_time) {
        *kernel_time = *reinterpret_cast<unsigned long long*>(&_kernel_time);
    }
    if (user_time) {
        *user_time = *reinterpret_cast<unsigned long long*>(&_user_time);
    }
}
void runner::run_process() {
    if (options.debug && !running_async) {
        run_process_async();
        WaitForSingleObject(running_thread, 100);//may stuck here
        WaitForSingleObject(init_semaphore, INFINITE);//may stuck here
        WaitForSingleObject(process_info.hProcess, INFINITE);//may stuck here
        return;
    }
    create_process();
    if (get_process_status() == process_spawner_crash || get_process_status() & process_finished_normal) {
        return;
    }
    running = true;
    requisites();
    if (get_process_status() == process_spawner_crash || get_process_status() & process_finished_normal) {
        return;
    }
    wait();
}

void runner::run_process_async() {
    running_async = true;
    running_thread = CreateThread(NULL, 0, async_body, this, 0, NULL);
}

bool runner::wait_for(const unsigned long &interval) {
    if (get_process_status() == process_spawner_crash || get_process_status() & process_finished_normal) {
        return true;
    }
    if (!running_async) {
        return false;
    }
    wait_for_init(interval);
    if (WaitForSingleObject(process_info.hProcess, interval) != WAIT_OBJECT_0) {
        return false;
    }
    WaitForSingleObject(running_thread, interval);
    CloseHandleSafe(running_thread);
    return true;
}

bool runner::wait_for_init(const unsigned long &interval) {
    while (init_semaphore == handle_default_value) {//not very good, made for synchro with async(mutex belongs to creator thread)
        Sleep(5);
    }
    return WaitForSingleObject(init_semaphore, interval) == WAIT_OBJECT_0;// TODO: get rid of this
}

void runner::safe_release() {
    process_status = process_spawner_crash;
    free();// make it safe!!!
}

void runner::set_pipe(const pipes_t &pipe_type, pipe_class *pipe_object) {
    pipes[pipe_type] = pipe_object;
}

pipe_class *runner::get_pipe(const pipes_t &pipe_type) {
    if (pipes.find(pipe_type) == pipes.end()) {
        return 0;
    }
    return pipes[pipe_type];
}
