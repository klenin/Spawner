#include "pipes.h"

CPipe::CPipe() :readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE)//init with default_handle_value
{
    Init();
}

CPipe::CPipe(std_pipe_t handleType) : readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE), pipe_type(handleType)
{
    Init();
    if (pipe_type & STD_PIPE_OI)
        SetHandleInformation(writePipe, HANDLE_FLAG_INHERIT, 0);
    if (pipe_type & STD_PIPE_IO)
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
}

void CPipe::Init()
{
    SECURITY_ATTRIBUTES saAttr;   
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0))
    {

    }
    //SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
}

void CPipe::ClosePipe()
{
    CloseHandleSafe(readPipe);
    CloseHandleSafe(writePipe);
}

CPipe::~CPipe()
{
    if (pipe_type & STD_PIPE_OI)
        CloseHandleSafe(readPipe);//replace with safe method
    if (pipe_type & STD_PIPE_IO)
        CloseHandleSafe(writePipe);//replace with safe method
}

bool CPipe::Write(void *data, size_t size)
{
    BOOL bSuccess;
    DWORD dwWritten;
    bSuccess = WriteFile(writePipe, data, size, &dwWritten, NULL);
    if (!bSuccess || dwWritten != size)
    {
        return false;
    }
    return true;
}

size_t CPipe::Read(void *data, size_t size)
{
    BOOL bSuccess;
    DWORD dwRead;
    bSuccess = ReadFile(readPipe, data, size, &dwRead, NULL);
    if (!bSuccess)
    {
        //error
    }
    return dwRead;
}
