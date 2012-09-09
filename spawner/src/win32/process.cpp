#include <process.h>
#include <iostream>

void CProcess::createProcess()
{
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = std_input.ReadPipe();
    si.hStdOutput = std_output.WritePipe();
    si.hStdError = std_error.WritePipe();

    si.lpDesktop = "";
    // TODO may be create new restriction for error handling
    // FIX not restriction, but option
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    if ( !CreateProcess(application.c_str(),
        "ls.exe -la",// replace with argument list construction
        NULL, NULL,
        TRUE,
        PROCESS_CREATION_FLAGS,
        NULL, NULL,
        &si, &process_info) )
    {
        throw("!!!");
    }
}

void CProcess::setRestrictions()
{
    /* implement restriction value check */
    hJob = CreateJobObject(NULL, NULL);
    DWORD le = GetLastError();

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

    if (restrictions.get_restriction(restriction_security_limit) != restriction_no_limit)
    {
        JOBOBJECT_BASIC_UI_RESTRICTIONS buir;
        buir.UIRestrictionsClass = JOB_OBJECT_UILIMIT_ALL;
        SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &buir, sizeof(buir));
    }
    // another option
    if (restrictions.get_restriction(restriction_gui_limit) != restriction_no_limit)
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
}

void CProcess::setupJobObject()
{
    AssignProcessToJobObject(hJob, process_info.hProcess);

    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 1, 1);

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp; 
    joacp.CompletionKey = (PVOID)COMPLETION_KEY; 
    joacp.CompletionPort = hIOCP; 
    SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
}

void CProcess::wait()
{
    DWORD waitTime = INFINITE;
    if (restrictions.get_restriction(restriction_user_time_limit) != restriction_no_limit)
    {
        waitTime = restrictions.get_restriction(restriction_user_time_limit);
        WaitForSingleObject(process_info.hProcess, waitTime); // TODO test this
        //and then terminate job object!!!
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
            //*message = TM_EXIT_PROCESS;
            break;
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            message++;
            process_status = process_finished_abnormally;
            //*message = TM_ABNORMAL_EXIT_PROCESS;
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            message++;
            //*message = TM_MEMORY_LIMIT_EXCEEDED;
            TerminateJobObject(hJob, 0);
            terminate_reason = terminate_reason_memory_limit;
            process_status = process_finished_terminated;
            break;
        };    

    } while (!message);
    std_output.wait_for_pipe(100);
    std_error.wait_for_pipe(100);
    WaitForSingleObject(process_info.hProcess, 10000);// TODO: get rid of this
    running = false;
}

void CProcess::finish()
{
    get_report();
    std_output.finish();
    std_error.finish();
    CloseHandleSafe(hIOCP);
    CloseHandleSafe(hJob);
    CloseHandleSafe(process_info.hProcess);
    CloseHandleSafe(process_info.hThread);
    CloseHandleSafe(check);
}

CProcess::CProcess(string file):application(file), process_status(process_not_started), terminate_reason(terminate_reason_not_terminated),
    std_input(STD_INPUT), std_output(STD_OUTPUT), std_error(STD_ERROR), running(false)
{
	//getting arguments from list
	//working dir, etc
}

void CProcess::SetArguments()
{
	//is this required?..
	//after-constructor argument changing
}

int CProcess::Run()
{
    // deprecated
    setRestrictions();
    createProcess();
    setupJobObject();
    running = true;

    DWORD w = ResumeThread(process_info.hThread);

    std_output.bufferize();
    std_error.bufferize();

    check = CreateThread(NULL, 0, check_limits, this, 0, NULL);
    // create thread, waiting for completition
    wait();
    int exit_code = get_exit_code();
    finish();

	return exit_code;
}

void CProcess::RunAsync()
{
    setRestrictions();    
    createProcess();
    setupJobObject();
    running = true;

    DWORD w = ResumeThread(process_info.hThread);

    std_output.bufferize();
    std_error.bufferize();

    check = CreateThread(NULL, 0, check_limits, this, 0, NULL);
    completition = CreateThread(NULL, 0, process_completition, this, 0, NULL);
    //WaitForSingleObject(completition, 100); // TODO fix this
    //create in another thread waiting function
}
CProcess::~CProcess()
{
	//kills process if it is running
}
thread_return_t CProcess::process_completition(thread_param_t param)
{
    CProcess *self = (CProcess *)param;
    self->wait();
    return 0;
}

thread_return_t CProcess::check_limits(thread_param_t param)
{
    CProcess *self = (CProcess *)param;
    DWORD t;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;

    if (self->restrictions.get_restriction(restriction_processor_time_limit) == restriction_no_limit &&
        self->restrictions.get_restriction(restriction_user_time_limit) == restriction_no_limit &&
        self->restrictions.get_restriction(restriction_write_limit) == restriction_no_limit)
        return 0;

    t = GetTickCount();
    while (1)
    {
        BOOL rs = QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL);
        if (!rs)
            break;
        //bai.BasicInfo.ThisPeriodTotalKernelTime

        if (self->restrictions.get_restriction(restriction_write_limit) != restriction_no_limit && 
            bai.IoInfo.WriteTransferCount > (1024 * 1024) * self->restrictions.get_restriction(restriction_write_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, NULL);
            break;
        }

        if (self->restrictions.get_restriction(restriction_processor_time_limit) != restriction_no_limit && 
            (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart > SECOND_COEFF * self->restrictions.get_restriction(restriction_processor_time_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);
            break;
        }
        if (self->restrictions.get_restriction(restriction_user_time_limit) != restriction_no_limit && 
            (GetTickCount() - t) > self->restrictions.get_restriction(restriction_user_time_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);//freezed
            break;
        }
        Sleep(1);
    }
    return 0;
}

unsigned long CProcess::get_exit_code()
{
    DWORD dwExitCode = 0;
    if (!GetExitCodeProcess(process_info.hProcess, &dwExitCode))
        throw "!!!";
    return dwExitCode;
}

void CProcess::suspend()
{
    if (get_process_status() != process_still_active)
        return;
    dumpThreads(true);
    process_status = process_suspended;
    //SuspendThread(process_info.hThread);
}

void CProcess::resume()
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

void CProcess::dumpThreads(bool suspend)
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
                    sizeof(te.th32OwnerProcessID) && te.th32OwnerProcessID == process_info.dwProcessId) 
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

process_status_t CProcess::get_process_status()
{
    // renew process status
    //cout << process_status << endl;
    // ************************* DIRTY HACK *************************//
    if (terminate_reason != terminate_reason_not_terminated)
        process_status = process_finished_terminated;
    // ************************* END OF HACK ************************//
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

istringstream & CProcess::stdoutput()
{
    return std_output.stream();
}

istringstream & CProcess::stderror()
{
    return std_error.stream();
}

bool CProcess::is_running()
{
    if (running)
        return process_is_active;
    return (bool)(get_process_status() & process_is_active);
}

exception_t CProcess::get_exception()
{
    if (get_process_status() == process_finished_abnormally)
        return (exception_t)get_exit_code();
    else return exception_no_exception;
}

CReport CProcess::get_report()
{
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
    if (hJob == INVALID_HANDLE_VALUE)
        return report;
    if (!QueryInformationJobObject(hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL))
    {
        //throw GetWin32Error("QueryInformationJobObject");
    }

    report.processor_time = bai.BasicInfo.TotalUserTime.QuadPart;
    report.write_transfer_count = bai.IoInfo.WriteTransferCount;
    //executionTime = (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart / SECOND_COEFF;
    //written = (DOUBLE)bai.IoInfo.WriteTransferCount / (1024 * 1024);

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION xli;
    if (!QueryInformationJobObject(hJob, JobObjectExtendedLimitInformation, &xli, sizeof(xli), NULL))
    {
        //throw GetWin32Error("QueryInformationJobObject");
    }

    report.peak_memory_used = xli.PeakJobMemoryUsed;

    report.process_status = get_process_status();
    report.exception = get_exception();
    report.terminate_reason = get_terminate_reason();
    report.exit_code = get_exit_code();
    return report;
}

terminate_reason_t CProcess::get_terminate_reason()
{
    return terminate_reason;
}

void CProcess::set_restrictions( const CRestrictions &Restrictions )
{
    // TODO m.b. test restrictions here
    restrictions = Restrictions;
}

void CProcess::Finish()
{
    finish();
}

bool CProcess::Wait(const unsigned long &ms_time)
{
    return WaitForSingleObject(completition, ms_time);
}