#include <process.h>
#include <iostream>

//TODO: implement setters and getters

CProcess::CProcess(/*arguments*/):stdinput(), stdoutput(), stderror()
{
	//getting arguments from list
	//working dir, etc
    for (int i = 0; i < RESTRICTION_MAX; i++)
        restrictions[i] = RESTRICTION_NO_LIMIT;
}

void CProcess::SetArguments()
{
	//is this required?..
	//after-constructor argument changing
}

int CProcess::Run(char *argv[])
{
	//runs a process with a given arguments and restrictions
	//probably returns children program return code
	//spawns new thread
	//and applies restrictions to it
	return 0;
}

void CProcess::RunAsync()
{
    STARTUPINFO        si;

    ZeroMemory(&si, sizeof(si));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdinput.ReadPipe();
    si.hStdOutput = stdoutput.WritePipe();
    si.hStdError = stderror.WritePipe();
    si.lpDesktop = "";
    SetErrorMode(SEM_FAILCRITICALERRORS);
    //si.wShowWindow =  (CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB);
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    if ( !CreateProcess( "j:\\Projects\\study\\fortune\\Debug\\voronoy.exe",//"C:\\GnuWin32\\bin\\ls.exe",
        NULL,
        NULL, NULL,
        TRUE,
        (CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB),
        NULL, NULL,
        &si, &process_info) )
    {
        throw("!!!");
    }
    //embed in function
    //BOOL rs = FALSE, rs1;
    //rs1 = IsProcessInJob(process_info.hProcess, NULL, &rs);
    hJob = CreateJobObject(NULL, NULL);
    DWORD le = GetLastError();

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
    memset(&joeli, 0, sizeof(joeli));
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
    
    if (GetRestrictionValue(RESTRICTION_MEMORY_LIMIT) != RESTRICTION_NO_LIMIT)
    {   
        joeli.JobMemoryLimit = GetRestrictionValue(RESTRICTION_MEMORY_LIMIT);
        joeli.ProcessMemoryLimit = GetRestrictionValue(RESTRICTION_MEMORY_LIMIT);
        joeli.BasicLimitInformation.LimitFlags |=
        JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
    }

    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli));
    le = GetLastError();
    if (GetRestrictionValue(RESTRICTION_SECURITY_LIMIT) != RESTRICTION_NO_LIMIT)
    {
        JOBOBJECT_BASIC_UI_RESTRICTIONS buir;
        buir.UIRestrictionsClass = JOB_OBJECT_UILIMIT_ALL;
        SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &buir, sizeof(buir));
    }

    AssignProcessToJobObject(hJob, process_info.hProcess);
    le = GetLastError();

    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 1, 1);
    le = GetLastError();
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp; 
    joacp.CompletionKey = (PVOID)COMPLETION_KEY; 
    joacp.CompletionPort = hIOCP; 
    SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
    le = GetLastError();
    DWORD w = ResumeThread(process_info.hThread);

    thread_t stdout_read = CreateThread(NULL, 0, read_body, this, 0, NULL);
    le = GetLastError();
    thread_t check = CreateThread(NULL, 0, check_limits, this, 0, NULL);
    le = GetLastError();
    DWORD waitTime = INFINITE;
    if (GetRestrictionValue(RESTRICTION_USER_TIME_LIMIT) != RESTRICTION_NO_LIMIT)
        waitTime = GetRestrictionValue(RESTRICTION_USER_TIME_LIMIT);
    WaitForSingleObject(process_info.hProcess, waitTime);
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
            break;
        case JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT:  
            message++;
            TerminateJobObject(hJob, 0);
            break;
        case JOB_OBJECT_MSG_EXIT_PROCESS:
            message++;
            //*message = TM_EXIT_PROCESS;
            break;
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            message++;
            //*message = TM_ABNORMAL_EXIT_PROCESS;
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            message++;
            //*message = TM_MEMORY_LIMIT_EXCEEDED;
            TerminateJobObject(hJob, 0);
            break;
        };    

    } while (!message);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

//
      //WaitForProcessTerminate(&terminateReason);  
    //WaitForSingleObject(stdout_read, INFINITE);
}
CProcess::~CProcess()
{
	//kills process if it is running
}
thread_return_t CProcess::process_body(thread_param_t param)
{

    return 0;
}

thread_return_t CProcess::read_body(thread_param_t param)
{
    CProcess *self = (CProcess *)param;
    for (;;)
    {
        char data[4096];
        size_t bytes_count = self->stdoutput.Read(data, 4096);
        if (bytes_count == 0)
            break;
        if (bytes_count != 0)
        {
            std::cout << bytes_count << std::endl;
            std::cout.write(data, bytes_count);
        }
    }
    return 0;
}

thread_return_t CProcess::check_limits(thread_param_t param)
{
    CProcess *self = (CProcess *)param;
    DWORD t;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;

    if (self->GetRestrictionValue(RESTRICTION_PROCESSOR_TIME_LIMIT) == RESTRICTION_NO_LIMIT &&
        self->GetRestrictionValue(RESTRICTION_USER_TIME_LIMIT) == RESTRICTION_NO_LIMIT &&
        self->GetRestrictionValue(RESTRICTION_WRITE_LIMIT) == RESTRICTION_NO_LIMIT)
        return 0;

    t = GetTickCount();
    while (1)
    {
        BOOL rs = QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL);
        if (!rs)
            break;

        if (self->GetRestrictionValue(RESTRICTION_WRITE_LIMIT) != RESTRICTION_NO_LIMIT && 
            bai.IoInfo.WriteTransferCount > (1024 * 1024) * self->GetRestrictionValue(RESTRICTION_WRITE_LIMIT))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, NULL);
            break;
        }

        if (self->GetRestrictionValue(RESTRICTION_PROCESSOR_TIME_LIMIT) != RESTRICTION_NO_LIMIT && 
            (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart > SECOND_COEFF * self->GetRestrictionValue(RESTRICTION_PROCESSOR_TIME_LIMIT))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);
            break;
        }
        if (self->GetRestrictionValue(RESTRICTION_USER_TIME_LIMIT) != RESTRICTION_NO_LIMIT && 
            (GetTickCount() - t) > self->GetRestrictionValue(RESTRICTION_USER_TIME_LIMIT))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);//freezed
            break;
        }
        Sleep(1);
  }
  return 0;
}