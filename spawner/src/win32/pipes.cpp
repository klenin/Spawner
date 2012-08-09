#include "pipes.h"

CPipe::CPipe() :readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE)//init with default_handle_value
{
    Init();
}

CPipe::CPipe(std_pipe_t handleType) : readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE), pipe_type(handleType), reading_thread(INVALID_HANDLE_VALUE), reading_mutex(INVALID_HANDLE_VALUE)
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


void CPipe::bufferize()
{
    if (reading_thread != INVALID_HANDLE_VALUE)
    {
        //error
        return;
    }        
    if (reading_mutex == INVALID_HANDLE_VALUE)
    {
        reading_mutex = CreateMutex(NULL, FALSE, NULL);
    }
    reading_thread = CreateThread(NULL, 0, reading_body, this, 0, NULL);
}

void CPipe::wait()
{
    if (reading_mutex == INVALID_HANDLE_VALUE || reading_thread == INVALID_HANDLE_VALUE)
        return;
    WaitForSingleObject(reading_mutex, INFINITE);
}

void CPipe::finish()
{
    WaitForSingleObject(reading_mutex, INFINITE);
    ReleaseMutex(reading_mutex);
    CloseHandleSafe(reading_thread);
    CloseHandleSafe(reading_mutex);
}

istringstream & CPipe::stream()
{
    wait();
    ReleaseMutex(reading_mutex);
    return reading_buffer;
}

size_t CPipe::buffer_size()
{
    return buff_size;
}

thread_return_t CPipe::reading_body(thread_param_t param)
{
    CPipe *self = (CPipe*)param;
    for (;;)
    {
        char data[BUFFER_SIZE];
        size_t bytes_count = self->Read(data, BUFFER_SIZE);
        if (bytes_count == 0)
            break;
        WaitForSingleObject(self->reading_mutex, INFINITE);
        if (bytes_count != 0)
        {
            data[bytes_count] = 0;
            self->buff_size += bytes_count;
            self->reading_buffer.str(self->reading_buffer.rdbuf()->str() + data);

            if (self->pipe_type == STD_OUTPUT)
                cout << "stdout :\n";
            if (self->pipe_type == STD_ERROR)
                cout << "stderr :\n";
            //if echo
            std::cout << bytes_count << std::endl;
            std::cout.write(data, bytes_count);
        }
        ReleaseMutex(self->reading_mutex);
    }
    return 0;
}
