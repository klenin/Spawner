#include <process.h>
#include <iostream>

//TODO: implement setters and getters

CProcess::CProcess(/*arguments*/):stdinput(), stdoutput(), stderror()
{
	//getting arguments from list
	//working dir, etc
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

    if ( !CreateProcess( "j:\\Projects\\study\\fortune\\Debug\\voronoy.exe",//"C:\\GnuWin32\\bin\\ls.exe",
        NULL,
        NULL, NULL,
        TRUE,
        (CREATE_SUSPENDED | CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW),
        NULL, NULL,
        &si, &process_info) )
    {
        throw("!!!");
    }
    DWORD w = ResumeThread(process_info.hThread);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    HANDLE thread1 = CreateThread(NULL, 0, read_body, this, 0, NULL);
    WaitForSingleObject(thread1, INFINITE);
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
