#include "pipes.h"
#include <error.h>
#include <iostream>
#include <fstream>
const unsigned int BUFFER_SIZE = 4096;

//move this to separate function
pipe_class::pipe_class(std_pipe_t handleType): 
    readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE), pipe_type(handleType), 
    buffer_thread(INVALID_HANDLE_VALUE), reading_mutex(INVALID_HANDLE_VALUE), state(true)
{
    //creating pipe
    SECURITY_ATTRIBUTES saAttr;   
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0))
    {
        raise_error(*this, "CreatePipe");
        return;
    }
    //setting inheritance
    HANDLE handle = writePipe;
    if (pipe_type & STD_OUTPUT)
        handle = readPipe;
    if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0))
    {
        raise_error(*this, "SetHandleInformation");
        return;
    }
}

void pipe_class::close_pipe()
{
    if (pipe_type & STD_INPUT)
        CloseHandleSafe(readPipe);
    if (pipe_type & STD_OUTPUT)
        CloseHandleSafe(writePipe);
    finish();
}

pipe_class::~pipe_class()
{
/*    if (pipe_type & STD_INPUT)
        CloseHandleSafe(readPipe);//replace with safe method
    if (pipe_type & STD_OUTPUT)
        CloseHandleSafe(writePipe);//replace with safe method*/
//    finish();
    close_pipe();
}

size_t pipe_class::write( void *data, size_t size )
{
    DWORD dwWritten;
    if (!WriteFile(writePipe, data, size, &dwWritten, NULL))// || dwWritten != size
    {
        raise_error(*this, "WriteFile");
        return 0;
    }
    if (!FlushFileBuffers(writePipe))
    {
        raise_error(*this, "FlushFileBuffers");
        return 0;
    }
    return dwWritten;
}

size_t pipe_class::read(void *data, size_t size)
{
    DWORD dwRead;
    if (!ReadFile(readPipe, data, size, &dwRead, NULL))
    {
        raise_error(*this, "ReadFile");
        return 0;
    }
    return dwRead;
}


bool pipe_class::bufferize()
{
    LPTHREAD_START_ROUTINE thread_start_routine = writing_buffer;
    if (buffer_thread != INVALID_HANDLE_VALUE)
    {
        //trying to bufferize twice
        return false;
    }        
    if (reading_mutex == INVALID_HANDLE_VALUE)
    {
        reading_mutex = CreateMutex(NULL, FALSE, NULL);
    }
    if (pipe_type & STD_OUTPUT)
        thread_start_routine = reading_buffer;
    buffer_thread = CreateThread(NULL, 0, thread_start_routine, this, 0, NULL);
    if (!buffer_thread)
    {
        raise_error(*this, "CreateThread");
        return false;
    }
    return true;
}

void pipe_class::wait()
{
    if (reading_mutex == INVALID_HANDLE_VALUE || buffer_thread == INVALID_HANDLE_VALUE)
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
    CloseHandleSafe(buffer_thread);
    CloseHandleSafe(reading_mutex);
}

void pipe_class::set_stream(const std::string &name)
{
    stream_name = name;
}

std::istringstream & pipe_class::stream()
{
    wait();
    ReleaseMutex(reading_mutex);
    return pipe_buffer;
}

size_t pipe_class::buffer_size()
{
    return buff_size;
}

thread_return_t pipe_class::writing_buffer(thread_param_t param)
{
    pipe_class *self = (pipe_class*)param;
    std::istream *is = NULL;

    if (self->stream_name.length() == 0)
        return 0;

    if (self->stream_name == "std")//replace with stderr, stdin and stdout correspondingly 
        is = &std::cin;
    else
        is = new std::ifstream(self->stream_name.c_str());

    char buff[BUFFER_SIZE + 1];
    while (!is->eof())
    {
        is->get(buff, BUFFER_SIZE);//only for TEXT data
        if (!self->write(buff, is->gcount()))
            break;
    }
    if (is && is != &std::cin)
        delete is;
    return 0;
}

thread_return_t pipe_class::reading_buffer(thread_param_t param)
{
    pipe_class *self = (pipe_class*)param;
    std::ostream *os = NULL;
    if (self->stream_name.length() == 0)
        return 0;
    if (self->stream_name == "std")
    {
        if (self->pipe_type == STD_OUTPUT)
            os = &std::cout;
        if (self->pipe_type == STD_ERROR)
            os = &std::cerr;
    }
    else
        os = new std::ofstream(self->stream_name.c_str());
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
            self->pipe_buffer.str(self->pipe_buffer.rdbuf()->str() + data);
            if (os)
            {
                os->write(data, bytes_count);
                os->flush();
            }
        }
        ReleaseMutex(self->reading_mutex);
    }
    if (os != &std::cerr && os != &std::cout)
        delete os;
    return 0;
}

void pipe_class::wait_for_pipe(const unsigned int &ms_time)
{
    WaitForSingleObject(buffer_thread, ms_time);
}

void pipe_class::safe_release()
{
    close_pipe();
    state = false;
}

bool pipe_class::valid()
{
    return state;
}
