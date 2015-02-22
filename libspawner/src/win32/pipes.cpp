#include "pipes.h"
#include <error.h>
#include <iostream>
#include <AccCtrl.h>
#include <Aclapi.h>//advapi32.lib

input_buffer_class dummy_input_buffer;
output_buffer_class dummy_output_buffer;
//move this to separate function

pipe_class::pipe_class(): pipe_type(PIPE_UNDEFINED), buffer_thread(handle_default_value), 
    reading_mutex(handle_default_value), readPipe(handle_default_value), writePipe(handle_default_value) {
    create_pipe();
}
pipe_class::pipe_class(const std_pipe_t &pipe_type): pipe_type(pipe_type), buffer_thread(handle_default_value), 
    reading_mutex(handle_default_value), readPipe(handle_default_value), writePipe(handle_default_value) {
    create_pipe();
}

pipe_t pipe_class::input_pipe() { 
    return readPipe;
}

pipe_t pipe_class::output_pipe() {
    return writePipe;
}

bool pipe_class::create_pipe() {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    
    if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0)) {
        raise_error(*this, "CreatePipe");
        return false;
    }
}

void pipe_class::close_pipe()
{
    finish();
    if (pipe_type == PIPE_INPUT) {
        CloseHandleSafe(readPipe);
    }
    if (pipe_type == PIPE_OUTPUT) {
        CloseHandleSafe(writePipe);
    }
}

pipe_class::~pipe_class()
{
    close_pipe();
}

size_t pipe_class::write(void *data, size_t size)
{
    DWORD dwWritten;
    if (!WriteFile(writePipe, data, size, &dwWritten, NULL))// || dwWritten != size
    {
        raise_error(*this, "WriteFile");
        return 0;
    }
/*    if (!FlushFileBuffers(writePipe))
    {
        raise_error(*this, "FlushFileBuffers");
        return 0;
    }*/
    return dwWritten;
}

size_t pipe_class::read(void *data, size_t size)
{
    DWORD dwRead;
    static DWORD total = 0;
    DWORD bytesAvailable = 0;
    /*
    */
    size_t i = 0;
    /*while (bytesAvailable <= size) {
        PeekNamedPipe(readPipe, NULL, size, NULL, &bytesAvailable, NULL);
        if (bytesAvailable && i > 5) {
            break;
        }
        i++;
        Sleep(1);
    }/**/
    //DWORD e;
    //std::cout << "r:" <<WaitCommEvent(readPipe, &e, NULL) << GetLastError std::endl;
    //std::cout << e << std::endl;
    if (!ReadFile(readPipe, data, size, &dwRead, NULL))
    {
        //raise_error(*this, "ReadFile");
        return 0;
    }
    total += dwRead;
    //        std::cout << total << " ";
    return dwRead;
}


bool pipe_class::bufferize()
{
    if (buffer_thread != handle_default_value)
    {
        //trying to bufferize twice
        return false;
    }        
    if (reading_mutex == handle_default_value)
    {
        reading_mutex = CreateMutex(NULL, FALSE, NULL);
    }
    return true;
}

void pipe_class::wait()
{
    if (reading_mutex == handle_default_value || buffer_thread == handle_default_value)
        return;
    WaitForSingleObject(reading_mutex, INFINITE);
}

void pipe_class::finish()
{
    if (reading_mutex != handle_default_value)
    {
        WaitForSingleObject(reading_mutex, INFINITE);
        ReleaseMutex(reading_mutex);
    }
    if (buffer_thread && buffer_thread != INVALID_HANDLE_VALUE) {
        TerminateThread(buffer_thread, 0);
        buffer_thread = INVALID_HANDLE_VALUE;
    }
    //CloseHandleSafe(buffer_thread);
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




thread_return_t input_pipe_class::writing_buffer(thread_param_t param) {
    input_pipe_class *self = (input_pipe_class*)param;

    char buffer[BUFFER_SIZE + 1];
    size_t read_count;
    if (self->writePipe == INVALID_HANDLE_VALUE) {
        return 0;
    }
    for (uint i = 0; i < self->input_buffers.size(); ++i) {
        if (!self->input_buffers[i]->readable()) {
            return 0;
        }
    }
    size_t can_continue = 1;
    while (can_continue) {
        can_continue = 0;
        for (uint i = 0; i < self->input_buffers.size(); ++i) {
            read_count = self->input_buffers[i]->read(buffer, BUFFER_SIZE);
            if (!self->write(buffer, read_count)) {
                return 0;
            }
            can_continue += read_count!=0;
        }
    }
    return 0;
}

input_pipe_class::input_pipe_class(): pipe_class(PIPE_INPUT) {
}

input_pipe_class::input_pipe_class(input_buffer_class *input_buffer_param): 
    pipe_class(PIPE_INPUT) {
    input_buffers.push_back(input_buffer_param);
}

input_pipe_class::~input_pipe_class() {
    TerminateThread(buffer_thread, 0);
}

input_pipe_class::input_pipe_class(std::vector<input_buffer_class *> input_buffer_param): 
    pipe_class(PIPE_INPUT), input_buffers(input_buffer_param) {
}

void input_pipe_class::add_input_buffer(input_buffer_class *input_buffer_param) {
    input_buffers.push_back(input_buffer_param);
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
	if (self->readPipe == INVALID_HANDLE_VALUE) {
        std::cout << "fail";
		return 0;
	}
    //if debug show some message

    for (uint i = 0; i < self->output_buffers.size(); ++i) {
	    if (!self->output_buffers[i]->writeable()) {
            std::cout << "fail";
			return 0;
		}
	}
    for (;;)
    {
        char data[BUFFER_SIZE];
        size_t bytes_count = self->read(data, BUFFER_SIZE);
        if (bytes_count == 0) {
            break;
        }
        WaitForSingleObject(self->reading_mutex, INFINITE);
        if (bytes_count != 0)
        {
            if (bytes_count < BUFFER_SIZE) {
                data[bytes_count] = 0;
            }
			for (uint i = 0; i < self->output_buffers.size(); ++i) {
				self->output_buffers[i]->write(data, bytes_count);
			}
        }
        ReleaseMutex(self->reading_mutex);
    }
            std::cout << "fail";
    return 0;
}

output_pipe_class::output_pipe_class(): pipe_class(PIPE_OUTPUT)
{}
output_pipe_class::output_pipe_class(output_buffer_class *output_buffer_param): 
    pipe_class(PIPE_OUTPUT)
{
	output_buffers.push_back(output_buffer_param);
}

output_pipe_class::~output_pipe_class() {
    TerminateThread(buffer_thread, 0);
}

output_pipe_class::output_pipe_class(std::vector<output_buffer_class *> output_buffer_param) :
    pipe_class(PIPE_OUTPUT), output_buffers(output_buffer_param)
{}

void output_pipe_class::add_output_buffer(output_buffer_class *output_buffer_param) {
    output_buffers.push_back(output_buffer_param);
}

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

