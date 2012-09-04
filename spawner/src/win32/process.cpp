#include <process.h>
#include <iostream>

HANDLE stdout_read_mutex = NULL;

//TODO: implement setters and getters

CAsyncProcess::CAsyncProcess(string file):application(file), proc_status(process_not_started), std_input(STD_INPUT), std_output(STD_OUTPUT), std_error(STD_ERROR)
{
	//getting arguments from list
	//working dir, etc
    for (int i = 0; i < restriction_max; i++)
        restrictions[i] = restriction_no_limit;
}

void CAsyncProcess::SetArguments()
{
	//is this required?..
	//after-constructor argument changing
}

int CAsyncProcess::Run()
{
    setRestrictions();    
    createProcess();
    setupJobObject();

    DWORD w = ResumeThread(process_info.hThread);

    std_output.bufferize();
    std_error.bufferize();

    check = CreateThread(NULL, 0, check_limits, this, 0, NULL);

    //create in another thread waiting function

	return 0;
}

void CAsyncProcess::RunAsync()
{
    // deprecated
    /*    setRestrictions();
    createProcess();
    setupJobObject();
    DWORD w = ResumeThread(process_info.hThread);

    std_output.bufferize();
    std_error.bufferize();

    check = CreateThread(NULL, 0, check_limits, this, 0, NULL);
    // create thread, waiting for completition
//    wait();
//    finish();*/
}
CAsyncProcess::~CAsyncProcess()
{
	//kills process if it is running
}
thread_return_t CAsyncProcess::process_body(thread_param_t param)
{

    return 0;
}

thread_return_t CAsyncProcess::check_limits(thread_param_t param)
{
    CAsyncProcess *self = (CAsyncProcess *)param;
    DWORD t;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;

    if (self->GetRestrictionValue(restriction_processor_time_limit) == restriction_no_limit &&
        self->GetRestrictionValue(restriction_user_time_limit) == restriction_no_limit &&
        self->GetRestrictionValue(restriction_write_limit) == restriction_no_limit)
        return 0;

    t = GetTickCount();
    while (1)
    {
        BOOL rs = QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL);
        if (!rs)
            break;

        if (self->GetRestrictionValue(restriction_write_limit) != restriction_no_limit && 
            bai.IoInfo.WriteTransferCount > (1024 * 1024) * self->GetRestrictionValue(restriction_write_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, NULL);
            break;
        }

        if (self->GetRestrictionValue(restriction_processor_time_limit) != restriction_no_limit && 
            (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart > SECOND_COEFF * self->GetRestrictionValue(restriction_processor_time_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);
            break;
        }
        if (self->GetRestrictionValue(restriction_user_time_limit) != restriction_no_limit && 
            (GetTickCount() - t) > self->GetRestrictionValue(restriction_user_time_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);//freezed
            break;
        }
        Sleep(1);
  }
  return 0;
}

void CAsyncProcess::createProcess()
{
    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = std_input.ReadPipe();
    si.hStdOutput = std_output.WritePipe();
    si.hStdError = std_error.WritePipe();

    si.lpDesktop = "";
    // TODO may be create new restriction for error handling
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

void CAsyncProcess::setRestrictions()
{
    /* implement restriction value check */
    hJob = CreateJobObject(NULL, NULL);
    DWORD le = GetLastError();

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
    memset(&joeli, 0, sizeof(joeli));
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;

    if (GetRestrictionValue(restriction_memory_limit) != restriction_no_limit)
    {   
        joeli.JobMemoryLimit = GetRestrictionValue(restriction_memory_limit);
        joeli.ProcessMemoryLimit = GetRestrictionValue(restriction_memory_limit);
        joeli.BasicLimitInformation.LimitFlags |=
            JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
    }

    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli));

    if (GetRestrictionValue(restriction_security_limit) != restriction_no_limit)
    {
        JOBOBJECT_BASIC_UI_RESTRICTIONS buir;
        buir.UIRestrictionsClass = JOB_OBJECT_UILIMIT_ALL;
        SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &buir, sizeof(buir));
    }

    if (GetRestrictionValue(restriction_gui_limit) != restriction_no_limit)
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
}

void CAsyncProcess::setupJobObject()
{
    AssignProcessToJobObject(hJob, process_info.hProcess);

    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 1, 1);

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp; 
    joacp.CompletionKey = (PVOID)COMPLETION_KEY; 
    joacp.CompletionPort = hIOCP; 
    SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
}

void CAsyncProcess::wait()
{
    DWORD waitTime = INFINITE;
    if (GetRestrictionValue(restriction_user_time_limit) != restriction_no_limit)
        waitTime = GetRestrictionValue(restriction_user_time_limit);
    WaitForSingleObject(process_info.hProcess, waitTime); // TODO test this
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
            proc_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT:  
            message++;
            TerminateJobObject(hJob, 0);
            proc_status = process_finished_terminated;
            break;
        case JOB_OBJECT_MSG_EXIT_PROCESS:
            message++;
            //*message = TM_EXIT_PROCESS;
            break;
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            message++;
            proc_status = process_finished_abnormally;
            //*message = TM_ABNORMAL_EXIT_PROCESS;
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            message++;
            //*message = TM_MEMORY_LIMIT_EXCEEDED;
            TerminateJobObject(hJob, 0);
            proc_status = process_finished_terminated;
            break;
        };    

    } while (!message);
    WaitForSingleObject(process_info.hProcess, 10000);// TODO
}

void CAsyncProcess::finish()
{
    std_output.finish();
    std_error.finish();
    CloseHandleSafe(hIOCP);
    CloseHandleSafe(hJob);
    CloseHandleSafe(process_info.hProcess);
    CloseHandleSafe(process_info.hThread);
    CloseHandleSafe(check);
}


CSimpleProcess::CSimpleProcess(string file):CAsyncProcess(file)
{
    //getting arguments from list
    //working dir, etc
    //modify options for pipes
}

int CSimpleProcess::Run()
{
    CAsyncProcess::Run();
    wait();
    finish();
    return 0;
}
