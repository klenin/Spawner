#include "pipes.h"
#include <error.h>
#include <iostream>
#include <fstream>
const unsigned int BUFFER_SIZE = 4096;//provide this in to constructor

//move this to separate function

pipe_class::pipe_class(): pipe_type(PIPE_UNDEFINED), buffer_thread(INVALID_HANDLE_VALUE), 
    reading_mutex(INVALID_HANDLE_VALUE) {
}
pipe_class::pipe_class(std_pipe_t pipe_type): pipe_type(pipe_type), buffer_thread(INVALID_HANDLE_VALUE), 
    reading_mutex(INVALID_HANDLE_VALUE) {
    create_pipe();
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

input_buffer_class dummy_input_buffer;
output_buffer_class dummy_output_buffer;

input_buffer_class::input_buffer_class(): buffer_size(BUFFER_SIZE) {
}

input_buffer_class::input_buffer_class(const size_t &buffer_size_param): buffer_size(buffer_size_param) {
}


output_buffer_class::output_buffer_class(): buffer_size(BUFFER_SIZE) {
}

output_buffer_class::output_buffer_class(const size_t &buffer_size_param): buffer_size(buffer_size_param) {
}

handle_buffer_class::handle_buffer_class(): stream(INVALID_HANDLE_VALUE) {
}

size_t handle_buffer_class::protected_read(void *data, size_t size) {
    DWORD bytes_read = 0;
    ReadFile(stream, data, size, &bytes_read, NULL);
    return bytes_read;
}
size_t handle_buffer_class::protected_write(void *data, size_t size) {
    DWORD bytes_written = 0;
    WriteFile(stream, data, size, &bytes_written, NULL);
    return bytes_written;
}
void handle_buffer_class::init_handle(handle_t stream_arg) {
    stream = stream_arg;
}
handle_buffer_class::~handle_buffer_class() {
    CloseHandleSafe(stream);
}


input_file_buffer_class::input_file_buffer_class(): input_buffer_class(), handle_buffer_class() {
}
input_file_buffer_class::input_file_buffer_class(const std::string &file_name, const size_t &buffer_size_param): 
    input_buffer_class(), handle_buffer_class() {
    handle_t handle = CreateFile(file_name.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (true){}
    init_handle(handle);
}
bool input_file_buffer_class::readable() {
    return (stream != INVALID_HANDLE_VALUE);
}
size_t input_file_buffer_class::read(void *data, size_t size) {
    return protected_read(data, size);
}

output_file_buffer_class::output_file_buffer_class(): output_buffer_class(), handle_buffer_class() {
}
output_file_buffer_class::output_file_buffer_class(const std::string &file_name, const size_t &buffer_size_param = BUFFER_SIZE): 
    output_buffer_class(buffer_size_param), handle_buffer_class() {
    handle_t handle = CreateFile(file_name.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (true){}
    init_handle(handle);
}
bool output_file_buffer_class::writeable() {
    return (stream != INVALID_HANDLE_VALUE);
}
size_t output_file_buffer_class::write(void *data, size_t size) {
    return protected_write(data, size);
}


output_stdout_buffer_class::output_stdout_buffer_class(): output_buffer_class(), handle_buffer_class() {
}
output_stdout_buffer_class::output_stdout_buffer_class(const size_t &buffer_size_param = BUFFER_SIZE): 
    output_buffer_class(buffer_size_param), handle_buffer_class() {
    handle_t handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (true){}
    init_handle(handle);
}
bool output_stdout_buffer_class::writeable() {
    return (stream != INVALID_HANDLE_VALUE);
}
size_t output_stdout_buffer_class::write(void *data, size_t size) {
    return protected_write(data, size);
}


thread_return_t input_pipe_class::writing_buffer(thread_param_t param) {
    input_pipe_class *self = (input_pipe_class*)param;

    if (!self->input_buffer->readable())
        return 0;

    char buffer[BUFFER_SIZE + 1];
    size_t read_count;
    while (read_count = self->input_buffer->read(buffer, BUFFER_SIZE)) {
        if (!self->write(buffer, read_count))
            break;
    }
    return 0;
}

input_pipe_class::input_pipe_class(): pipe_class(PIPE_INPUT), input_buffer(&dummy_input_buffer){
}

input_pipe_class::input_pipe_class(input_buffer_class *input_buffer_param): 
    pipe_class(PIPE_INPUT), input_buffer(input_buffer_param){
}

bool input_pipe_class::bufferize()
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

pipe_t input_pipe_class::get_pipe()
{
    return input_pipe();
}


thread_return_t output_pipe_class::reading_buffer(thread_param_t param)
{
    output_pipe_class *self = (output_pipe_class*)param;
    std::ostream *os = NULL;
    if (!self->output_buffer->writeable())
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
            self->output_buffer->write(data, bytes_count);
        }
        ReleaseMutex(self->reading_mutex);
    }
    return 0;
}

output_pipe_class::output_pipe_class(): pipe_class(PIPE_OUTPUT), output_buffer(&dummy_output_buffer)
{}
output_pipe_class::output_pipe_class(output_buffer_class *output_buffer_param): 
    pipe_class(PIPE_OUTPUT), output_buffer(output_buffer_param)
{}

bool output_pipe_class::bufferize()
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

pipe_t output_pipe_class::get_pipe()
{
    return output_pipe();
}

