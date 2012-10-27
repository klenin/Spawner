#include "pipes.h"

pipe_class::pipe_class() :readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE)//init with default_handle_value
{
    init();
}

pipe_class::pipe_class(std_pipe_t handleType) : readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE), pipe_type(handleType), reading_thread(INVALID_HANDLE_VALUE), reading_mutex(INVALID_HANDLE_VALUE)
{
    init();
    if (pipe_type & STD_PIPE_OI)
        SetHandleInformation(writePipe, HANDLE_FLAG_INHERIT, 0);
    if (pipe_type & STD_PIPE_IO)
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
}

void pipe_class::init()
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

void pipe_class::close_pipe()
{
    CloseHandleSafe(readPipe);
    CloseHandleSafe(writePipe);
}

pipe_class::~pipe_class()
{
    if (pipe_type & STD_PIPE_OI)
        CloseHandleSafe(readPipe);//replace with safe method
    if (pipe_type & STD_PIPE_IO)
        CloseHandleSafe(writePipe);//replace with safe method
}

bool pipe_class::write(void *data, size_t size)
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

size_t pipe_class::read(void *data, size_t size)
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


void pipe_class::bufferize()
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

void pipe_class::wait()
{
    if (reading_mutex == INVALID_HANDLE_VALUE || reading_thread == INVALID_HANDLE_VALUE)
        return;
    WaitForSingleObject(reading_mutex, INFINITE);
}

void pipe_class::finish()
{
    if (reading_mutex != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(reading_mutex, INFINITE);
        ReleaseMutex(reading_mutex);
    }
    CloseHandleSafe(reading_thread);
    CloseHandleSafe(reading_mutex);
}

std::istringstream & pipe_class::stream()
{
    wait();
    ReleaseMutex(reading_mutex);
    return reading_buffer;
}

size_t pipe_class::buffer_size()
{
    return buff_size;
}

thread_return_t pipe_class::reading_body(thread_param_t param)
{
    pipe_class *self = (pipe_class*)param;
    for (;;)
    {
        char data[BUFFER_SIZE];
        size_t bytes_count = self->read(data, BUFFER_SIZE);
        if (bytes_count == 0)
            break;
        WaitForSingleObject(self->reading_mutex, INFINITE);
        if (bytes_count != 0)
        {
            data[bytes_count] = 0;
            self->buff_size += bytes_count;
            self->reading_buffer.str(self->reading_buffer.rdbuf()->str() + data);

            if (self->pipe_type == STD_OUTPUT)
                std::cout << "stdout :\n";
            if (self->pipe_type == STD_ERROR)
                std::cout << "stderr :\n";
            //if echo
            std::cout << bytes_count << std::endl;
            std::cout.write(data, bytes_count);
        }
        ReleaseMutex(self->reading_mutex);
    }
    return 0;
}

void pipe_class::wait_for_pipe(const unsigned int &ms_time)
{
    WaitForSingleObject(reading_thread, ms_time);
}
