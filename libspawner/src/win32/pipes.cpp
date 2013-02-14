#include "pipes.h"
#include <error.h>
#include <iostream>
#include <fstream>
const unsigned int BUFFER_SIZE = 4096;//provide this in to constructor

//move this to separate function
pipe_class::pipe_class(std_pipe_t pipe_type): pipe_type(pipe_type)
{
    create_pipe();
}

pipe_class::pipe_class(std::string file_name, std_pipe_t pipe_type): pipe_type(pipe_type), file_name* {
}

bool pipe_class::create_pipe()
{
    readPipe = INVALID_HANDLE_VALUE;
    writePipe = INVALID_HANDLE_VALUE;
    buffer_thread = INVALID_HANDLE_VALUE;
    reading_mutex = INVALID_HANDLE_VALUE;
    state = true;
    SECURITY_ATTRIBUTES saAttr;   
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0))
    {
        raise_error(*this, "CreatePipe");
        return false;
    }
    //setting inheritance
    HANDLE handle = writePipe;
    if (pipe_type == PIPE_OUTPUT)
        handle = readPipe;
    if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0))
    {
        raise_error(*this, "SetHandleInformation");
        return false;
    }
    return true;
}

void pipe_class::close_pipe()
{
    if (pipe_type & PIPE_INPUT)
        CloseHandleSafe(readPipe);
    if (pipe_type & PIPE_OUTPUT)
        CloseHandleSafe(writePipe);
    finish();
}

pipe_class::~pipe_class()
{
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
    if (buffer_thread != INVALID_HANDLE_VALUE)
    {
        //trying to bufferize twice
        return false;
    }        
    if (reading_mutex == INVALID_HANDLE_VALUE)
    {
        reading_mutex = CreateMutex(NULL, FALSE, NULL);
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

pipe_t pipe_class::get_pipe()
{
    return 0;
}

thread_return_t read_pipe_class::writing_buffer(thread_param_t param)
{
    read_pipe_class *self = (read_pipe_class*)param;

    if (self->input_stream == dummy_istream)
        return 0;

    char buff[BUFFER_SIZE + 1];
    while (!self->input_stream.eof())
    {
        self->input_stream.get(buff, BUFFER_SIZE);
        if (!self->write(buff, self->input_stream.gcount()))
            break;
    }
    return 0;
}

read_pipe_class::read_pipe_class(): pipe_class(PIPE_INPUT), input_stream(dummy_istream)
{}
read_pipe_class::read_pipe_class(std::istream &input_stream): pipe_class(PIPE_INPUT), input_stream(input_stream)
{}

bool read_pipe_class::bufferize()
{
    if (!pipe_class::bufferize())
        return false;
    buffer_thread = CreateThread(NULL, 0, writing_buffer, this, 0, NULL);
    if (!buffer_thread)
    {
        raise_error(*this, "CreateThread");
        return false;
    }
    return true;
}

pipe_t read_pipe_class::get_pipe()
{
    return read_pipe();
}


thread_return_t write_pipe_class::reading_buffer(thread_param_t param)
{
    write_pipe_class *self = (write_pipe_class*)param;
    std::ostream *os = NULL;
    if (self->output_stream == dummy_ostream)
        return 0;
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
            self->output_stream.write(data, bytes_count);
            self->output_stream.flush();
        }
        ReleaseMutex(self->reading_mutex);
    }
    return 0;
}

write_pipe_class::write_pipe_class(): pipe_class(PIPE_OUTPUT), output_stream(dummy_ostream)
{}
write_pipe_class::write_pipe_class(std::ostream &output_stream): pipe_class(PIPE_OUTPUT), output_stream(output_stream)
{}

bool write_pipe_class::bufferize()
{
    if (!pipe_class::bufferize())
        return false;
    buffer_thread = CreateThread(NULL, 0, reading_buffer, this, 0, NULL);
    if (!buffer_thread)
    {
        raise_error(*this, "CreateThread");
        return false;
    }
    return true;
}

pipe_t write_pipe_class::get_pipe()
{
    return write_pipe();
}

