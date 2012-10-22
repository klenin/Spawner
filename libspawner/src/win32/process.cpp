#include <process.h>
#include <iostream>
#include <windows.h>
#include <winbase.h>
#include <time.h>
#include <vector>
const size_t MAX_RATE_COUNT = 20;

/*thread_return_t CProcess::debug_thread_proc(thread_param_t param)
{
    CProcess *self = (CProcess *)param;
    ZeroMemory(&self->si, sizeof(self->si));

    self->si.cb = sizeof(self->si);
    self->si.dwFlags = STARTF_USESTDHANDLES;
    self->si.hStdInput = self->std_input.ReadPipe();
    self->si.hStdOutput = self->std_output.WritePipe();
    self->si.hStdError = self->std_error.WritePipe();
    DWORD process_creation_flags = PROCESS_CREATION_FLAGS;
    self->si.lpDesktop = "";

    if (self->options.hide_gui)
    {
        self->si.dwFlags |= STARTF_USESHOWWINDOW;
        self->si.wShowWindow = SW_HIDE;
    }

    // Extracting program name and generating cmd line
    char *cmd;
    const char *wd = (self->options.working_directory != "")?self->options.working_directory.c_str():NULL;
    string command_line;
    size_t  index_win = self->application.find_last_of('\\'), 
        index_nix = self->application.find_last_of('/');

    if (index_win != string::npos)
        command_line = self->application.substr(index_win + 1);
    else if (index_nix != string::npos)
        command_line = self->application.substr(index_nix + 1);
    else
        command_line = self->application;

    command_line = command_line + " " + (self->options.string_arguments==""?self->options.get_arguments():self->options.string_arguments);
    cmd = new char [command_line.size()+1];
    strcpy(cmd, command_line.c_str());

    if (self->options.silent_errors)
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        process_creation_flags |= DEBUG_PROCESS;
    }

    // check if program exists or smth like this
    // if not exists try to execute full cmd
    if ( !CreateProcess(self->application.c_str(),
        cmd,
        NULL, NULL,
        TRUE,
        process_creation_flags,
        NULL, wd,
        &self->si, &self->process_info) )
    {
        if ( !CreateProcess(NULL,
            cmd,
            NULL, NULL,
            TRUE,
            process_creation_flags,
            NULL, wd,
            &self->si, &self->process_info) )
        {
            DWORD le = GetLastError();
            delete[] cmd;
            throw("!!!");
        }
    }
    delete[] cmd;
    DEBUG_EVENT debug_event;
    WaitForSingleObject(self->process_initiated, 0);
    while (WaitForDebugEvent(&debug_event, INFINITE))
    {
        ContinueDebugEvent(debug_event.dwProcessId,
            debug_event.dwThreadId,
            DBG_CONTINUE);
        if (debug_event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            debug_event.dwProcessId = 0;
        }
        //std::cout << debug_event.u.DebugString.lpDebugStringData;
    }

    return 0;
}

bool CProcess::set_terminate_reason(const terminate_reason_t &reason)
{
    terminate_reason_t current_reason = get_terminate_reason();
    if (current_reason != terminate_reason_not_terminated || reason == current_reason)
        return false;

    return true;
}

process_id CProcess::get_process_id()
{
    if (get_process_status() == process_not_started)
        return 0;//error
    return process_info.dwProcessId;
}

istringstream & CProcess::stdoutput()
{
    return std_output.stream();
}

istringstream & CProcess::stderror()
{
    return std_error.stream();
}

bool CProcess::Wait(const unsigned long &ms_time)
{
    return WaitForSingleObject(completition, ms_time) != 0;
}
*/

void process_class::init_process(char *cmd, const char *wd)
{
    if ( !CreateProcess(program.c_str(),
        cmd,
        NULL, NULL,
        TRUE,
        process_creation_flags,
        NULL, wd,
        &si, &process_info) )
    {
        if ( !CreateProcess(NULL,
            cmd,
            NULL, NULL,
            TRUE,
            process_creation_flags,
            NULL, wd,
            &si, &process_info) )
        {
            DWORD le = GetLastError();
            process_status = process_failed_to_create;
        }
    }
}

void process_class::create_process()
{
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = std_input.read_pipe();
    si.hStdOutput = std_output.write_pipe();
    si.hStdError = std_error.write_pipe();
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
    const char *wd = (options.working_directory != "")?options.working_directory.c_str():NULL;
    string command_line;
    size_t  index_win = program.find_last_of('\\'), 
        index_nix = program.find_last_of('/');

    if (index_win != string::npos)
        command_line = program.substr(index_win + 1);
    else if (index_nix != string::npos)
        command_line = program.substr(index_nix + 1);
    else
        command_line = program;

    command_line = command_line + " " + (options.string_arguments==""?options.get_arguments():options.string_arguments);
    cmd = new char [command_line.size()+1];
    strcpy(cmd, command_line.c_str());
    init_process(cmd, wd);
    delete[] cmd;
}

process_class::process_class(const std::string &program, const options_class &options) :program(program), options(options), std_input(STD_INPUT), std_output(STD_OUTPUT), std_error(STD_ERROR)
{
    create_process();
}

process_class::~process_class()
{
    std_output.finish();
    std_error.finish();
    CloseHandleSafe(process_info.hProcess);
    CloseHandleSafe(process_info.hThread);
    free();
}

unsigned long process_class::get_exit_code()
{
    DWORD dwExitCode = 0;
    if (!GetExitCodeProcess(process_info.hProcess, &dwExitCode))
        throw "!!!";
    return dwExitCode;
}

process_status_t process_class::get_process_status()
{
    // renew process status
    if (process_status == process_failed_to_create)
        return process_status;
    if (process_status & process_finished || process_status == process_suspended)
        return process_status;
    unsigned long exitcode = get_exit_code();
    if (exitcode == exit_code_still_active)
        process_status = process_still_active;
    else
        process_status = process_finished_abnormally;
    if (exitcode == 0)
        process_status = process_finished_normal;
    return process_status;
}

exception_t process_class::get_exception()
{
    if (get_process_status() == process_finished_abnormally)
        return (exception_t)get_exit_code();
    else return exception_exception_no;
}

handle_t process_class::get_handle()
{
    return process_info.hProcess;
}

unsigned long process_class::get_id()
{
    return process_info.dwProcessId;
}

std::string process_class::get_program() const
{
    return program;
}

options_class process_class::get_options() const
{
    return options;
}

void process_class::run_process()
{
    if (get_process_status() == process_failed_to_create)
        return;
    ResumeThread(process_info.hThread);
    std_output.bufferize();
    std_error.bufferize();
}

void process_class::wait( const unsigned long &interval )
{
    WaitForSingleObject(process_info.hProcess, interval);// TODO: get rid of this
    std_output.wait_for_pipe(100);
    std_error.wait_for_pipe(100);
}

void process_wrapper::apply_restrictions()
{
    /* implement restriction value check */
    hJob = CreateJobObject(NULL, NULL);
    DWORD le = GetLastError();

    // Memory and time limit
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
    memset(&joeli, 0, sizeof(joeli));
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;

    if (restrictions.get_restriction(restriction_memory_limit) != restriction_no_limit)
    {   
        joeli.JobMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
        joeli.ProcessMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
        joeli.BasicLimitInformation.LimitFlags |=
            JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
    }

    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli));

    // Security limit
    if (restrictions.get_restriction(restriction_security_limit) != restriction_no_limit)
    {
        JOBOBJECT_BASIC_UI_RESTRICTIONS buir;
#ifdef JOB_OBJECT_UILIMIT_ALL
        buir.UIRestrictionsClass = JOB_OBJECT_UILIMIT_ALL;
#else
        buir.UIRestrictionsClass = 0x000000FF;
#endif
        SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &buir, sizeof(buir));
    }

    // Assigning created process to job object
    AssignProcessToJobObject(hJob, process.get_handle());

    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 1, 1);

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp; 
    joacp.CompletionKey = (PVOID)COMPLETION_KEY; 
    joacp.CompletionPort = hIOCP; 
    SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
}

thread_return_t process_wrapper::process_completition_proc( thread_param_t param )
{
    process_wrapper *self = (process_wrapper*)param;
    self->wait();
    return 0;
}

thread_return_t process_wrapper::check_limits_proc( thread_param_t param )
{
    process_wrapper *self = (process_wrapper*)param;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
    restrictions_class restrictions = self->restrictions;

    if (restrictions.get_restriction(restriction_processor_time_limit) == restriction_no_limit &&
        restrictions.get_restriction(restriction_user_time_limit) == restriction_no_limit &&
        restrictions.get_restriction(restriction_write_limit) == restriction_no_limit &&
        restrictions.get_restriction(restriction_load_ratio) == restriction_no_limit)
        return 0;

    DWORD t;
    int dt;
    double total_rate = 10000.0;
    LONGLONG last_quad_part = 0;
    self->report.load_ratio = 10000.0;
    std::vector<double> rates;
    static const double sec_clocks = (double)1000.0/CLOCKS_PER_SEC;
    rates.push_back(total_rate);

    t = GetTickCount();
    dt = clock();
    while (1)
    {
        if (!QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL))
            break;

        if (restrictions.get_restriction(restriction_write_limit) != restriction_no_limit && 
            bai.IoInfo.WriteTransferCount > restrictions.get_restriction(restriction_write_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, NULL);
            break;
        }

        if (restrictions.get_restriction(restriction_processor_time_limit) != restriction_no_limit && 
            (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart > SECOND_COEFF * restrictions.get_restriction(restriction_processor_time_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);
            break;
        }
        if (restrictions.get_restriction(restriction_user_time_limit) != restriction_no_limit && 
            (GetTickCount() - t) > restrictions.get_restriction(restriction_user_time_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT, COMPLETION_KEY, NULL);//freezed
            break;
        }
        if ((clock() - dt)*sec_clocks > 100.0 && bai.BasicInfo.TotalUserTime.QuadPart)
        {
            //change to time limit
            double load_ratio = (double)(bai.BasicInfo.TotalUserTime.QuadPart - last_quad_part)/(sec_clocks*(clock() - dt));
            rates.push_back(load_ratio);// make everything integer
            if (rates.size() >= MAX_RATE_COUNT)
            {
                total_rate -= rates[0];
                rates.erase(rates.begin());
            }
            total_rate += load_ratio;
            self->report.load_ratio = total_rate/rates.size();
            last_quad_part = bai.BasicInfo.TotalUserTime.QuadPart;
            dt = clock();
            if (restrictions.get_restriction(restriction_load_ratio) != restriction_no_limit)
            {
                if (self->report.load_ratio < 0.01*self->restrictions.get_restriction(restriction_load_ratio))
                {
                    PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT, COMPLETION_KEY, NULL);//freezed
                    break;
                }
            }
        }
        Sleep(1);
    }
    return 0;
}

void process_wrapper::dump_threads( bool suspend )
{
    //if process is active and started!!!
    if (!is_running())
        return;
    //while (threads.empty())
    //{
    //CloseHandle(threads.begin()
    //}
    threads.clear();
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        if (Thread32First(h, &te)) 
        {
            do {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
                    sizeof(te.th32OwnerProcessID) && te.th32OwnerProcessID == process.get_id()) 
                {
                    handle_t handle = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                    if (suspend)
                        SuspendThread(handle);
                    //may be close here??
                    threads.push_back(handle);
                    /*printf("Process 0x%04x Thread 0x%04x\n",
                    te.th32OwnerProcessID, te.th32ThreadID);*/
                }
                te.dwSize = sizeof(te);
            } while (Thread32Next(h, &te));
        }
        CloseHandle(h);
    }
}

void process_wrapper::wait()
{
    clock_t program_run_time = clock();//TODO:make this global
    if (restrictions.get_restriction(restriction_user_time_limit) == restriction_no_limit)
    {
        //WaitForSingleObject(process_info.hProcess, INFINITE);
        // careful with this comment - it may cause problems with make
    }
    DWORD dwNumBytes, dwKey;
    LPOVERLAPPED completedOverlapped;  
    static CHAR buf[1024];
    //set msg
    int message = 0;
    do
    {     
        GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);

        switch (dwNumBytes)
        {
        case JOB_OBJECT_MSG_NEW_PROCESS:
            break;
        case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
            message++;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_time_limit;
            process_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT:  
            message++;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_write_limit;
            process_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_EXIT_PROCESS:
            message++;
            break;
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            message++;
            process_status = process_finished_abnormally;
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            message++;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_memory_limit;
            process_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
            message++;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_memory_limit;
            process_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT:
            message++;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_user_time_limit;
            process_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT:
            message++;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_load_ratio_limit;
            process_status = process_finished_terminated;
            break;
        };
        //cout << dwNumBytes;

    } while (!message);
    program_run_time = clock() - program_run_time;
    cout << program_run_time;
    report.user_time = (program_run_time*1000)/CLOCKS_PER_SEC;
    GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);
    process.wait(INFINITE);
    running = false;
}

process_wrapper::process_wrapper( const std::string &program, const options_class &options, const restrictions_class &restrictions ) :process(program, options), restrictions(restrictions)
{
    apply_restrictions();
}

process_wrapper::~process_wrapper()
{
    CloseHandleSafe(hIOCP);
    CloseHandleSafe(hJob);
    CloseHandleSafe(check_thread);
}

void process_wrapper::run_process()
{
    if (get_process_status() == process_failed_to_create || get_process_status() & process_finished)
        return;
    process.run_process();
    running = true;
    check_thread = CreateThread(NULL, 0, check_limits_proc, this, 0, NULL);
    wait();
}

void process_wrapper::run_async()
{
    if (get_process_status() == process_failed_to_create || get_process_status() & process_finished)
        return;
    process.run_process();
    running = true;
    check_thread = CreateThread(NULL, 0, check_limits_proc, this, 0, NULL);
    completition = CreateThread(NULL, 0, process_completition_proc, this, 0, NULL);
    //WaitForSingleObject(completition, 100); // TODO fix this
    //create in another thread waiting function
}

terminate_reason_t process_wrapper::get_terminate_reason()
{
    return terminate_reason;
}

report_class process_wrapper::get_report()
{
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
    report.process_status = get_process_status();
    if (hJob == INVALID_HANDLE_VALUE || get_process_status() == process_failed_to_create)
        return report;
    if (!QueryInformationJobObject(hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL))
    {
        //throw GetWin32Error("QueryInformationJobObject");
    }

    report.processor_time = bai.BasicInfo.TotalUserTime.QuadPart;
    report.kernel_time = bai.BasicInfo.TotalKernelTime.QuadPart;
    report.write_transfer_count = bai.IoInfo.WriteTransferCount;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION xli;
    if (!QueryInformationJobObject(hJob, JobObjectExtendedLimitInformation, &xli, sizeof(xli), NULL))
    {
        //throw GetWin32Error("QueryInformationJobObject");
    }

    report.peak_memory_used = xli.PeakJobMemoryUsed;

    report.application_name = process.get_program();

    report.exception = process.get_exception();
    report.terminate_reason = get_terminate_reason();
    report.exit_code = process.get_exit_code();

    report.options = process.get_options();
    report.restrictions = restrictions;

    return report;
}

process_status_t process_wrapper::get_process_status()
{
    if (process_status == process_finished_terminated || process_status == process_suspended)
        return process_status;
    return process.get_process_status();
}

bool process_wrapper::is_running()
{
    if (running)
        return true;
    return (get_process_status() & process_is_active) != 0;
}

void process_wrapper::suspend()
{
    if (get_process_status() != process_still_active)
        return;
    dump_threads(true);
    process_status = process_suspended;
    //SuspendThread(process_info.hThread);
}

void process_wrapper::resume()
{
    if (get_process_status() != process_suspended)
        return;
    while (!threads.empty())
    {
        handle_t handle = threads.front();
        threads.pop_front();
        ResumeThread(handle);
        CloseHandle(handle);
    }
    process_status = process_still_active;
    get_process_status();
}