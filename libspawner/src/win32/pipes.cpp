#include "pipes.h"

#include <iostream>
#include <AccCtrl.h>
#include <Aclapi.h>//advapi32.lib
#include <algorithm>

#include "error.h"

// TODO: 4096 is assumed to be "enough" however if message sent via pipes will
// exceed buffer size then it's fragment could be mixed up with fragments of
// another messages. This is valid for fill_pipe_thread, and fixed for drain_pipe_thread.
// I tested it with a value of 8 and messages' parts are really being mixed up.
unsigned const DEFAULT_BUFFER_SIZE = 4096;

pipe_c::pipe_c()
    : pipe_type(PIPE_UNDEFINED)
    , buffer_thread(handle_default_value)
    , readPipe(handle_default_value)
    , writePipe(handle_default_value) {

    create_pipe();
}

pipe_c::pipe_c(const std_pipe_t &pipe_type)
    : pipe_type(pipe_type)
    , buffer_thread(handle_default_value)
    , readPipe(handle_default_value)
    , writePipe(handle_default_value) {

    create_pipe();
}

pipe_t pipe_c::input_pipe() {
    return readPipe;
}

pipe_t pipe_c::output_pipe() {
    return writePipe;
}

#include <io.h>
#include <fcntl.h>
void pipe_c::create_pipe()
{
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (CreatePipe(&readPipe, &writePipe, &saAttr, 0) == 0) {
        PANIC(get_win_last_error_string());
    }
}

void pipe_c::close_pipe() {
    if (pipe_type == PIPE_INPUT) {
        CloseHandleSafe(readPipe);
    }
    if (pipe_type == PIPE_OUTPUT) {
        CloseHandleSafe(writePipe);
    }
    finish();
}

pipe_c::~pipe_c() {
    close_pipe();
}

size_t pipe_c::write(const void *data, size_t size) {
    DWORD dwWritten;
    write_mutex.lock();
    // WriteFile is also return 0 when GetLastError() == ERROR_IO_PENDING
    if (WriteFile(writePipe, data, size, &dwWritten, NULL) == 0) {
        PANIC(get_win_last_error_string());
    }
    write_mutex.unlock();
    // TODO: dwWritten != size ???
    FlushFileBuffers(writePipe);
    return dwWritten;
}

size_t pipe_c::read(void *data, size_t size) {
    DWORD dwRead;
    if (!ReadFile(readPipe, data, size, &dwRead, NULL))
    {
        return 0;
    }
    return dwRead;
}

void pipe_c::bufferize() {
    // trying to bufferize twice
    PANIC_IF(buffer_thread != handle_default_value);
}

void pipe_c::finish()
{
    if (buffer_thread && buffer_thread != INVALID_HANDLE_VALUE) {
        buffer_thread = INVALID_HANDLE_VALUE;
    }
}

bool pipe_c::valid()
{
    return state;
}

pipe_t pipe_c::get_pipe()
{
    return 0;
}

void pipe_c::wait()
{
    if (buffer_thread == handle_default_value) {
        return;
    }
    stop_thread_ = true;
    // TODO: WAIT_TIMEOUT ?
    while (WaitForSingleObject(buffer_thread, 0) != WAIT_OBJECT_0) {
        CancelSynchronousIo_wrapper(buffer_thread);
    }
    CloseHandle(writePipe);
    CloseHandle(readPipe);
    writePipe = nullptr;
    readPipe = nullptr;
    if (this->pipe_type == PIPE_INPUT) {
        done_io_ = true;
    }
    while (!done_io_) {
        Sleep(1);
    }
}

thread_return_t input_pipe_c::fill_pipe_thread(thread_param_t param) {
    input_pipe_c *self = (input_pipe_c*)param;

    char buffer[DEFAULT_BUFFER_SIZE + 1];
    size_t read_count;
    if (self->writePipe == INVALID_HANDLE_VALUE) {
        self->done_io_ = true;
        return 0;
    }
    for (uint i = 0; i < self->input_buffers.size(); ++i) {
        if (!self->input_buffers[i]->readable()) {
            self->done_io_ = true;
            return 0;
        }
    }

    while (true) {
        if (self->stop_thread_) {
            break;
        }
        self->buffers_mutex_.lock();
        for (uint i = 0; i < self->input_buffers.size(); ++i) {
            self->last_buffer_ = self->input_buffers[i];
            if (self->input_buffers[i]->peek() > 0) {
                read_count = self->input_buffers[i]->read(buffer, DEFAULT_BUFFER_SIZE);
                if (!self->write(buffer, read_count)) {
                    self->buffers_mutex_.unlock();
                    self->done_io_ = true;
                    return 0;
                }
            }
            else {
                Sleep(10);
            }
        }
        Sleep(10);
        self->buffers_mutex_.unlock();
    }
    self->done_io_ = true;
    return 0;
}

input_pipe_c::input_pipe_c()
    : pipe_c(PIPE_INPUT) {

}

input_pipe_c::~input_pipe_c() {
    wait();
}

void input_pipe_c::add_input_buffer(std::shared_ptr<input_buffer_c> input_buffer_param) {
    input_buffers.push_back(input_buffer_param);
    buffers_.push_back(input_buffer_param);
}

void input_pipe_c::bufferize()
{
    // we can only save one thread if there's no input buffers
    // threads for stdout and stderr couldn't be saved since we have to consume
    // the pipe in order for the application not to block on printf
    if (input_buffers.empty()) {
        return;
    }
    pipe_c::bufferize();
    buffer_thread = CreateThread(NULL, 0, fill_pipe_thread, this, 0, NULL);
    if (buffer_thread == NULL) {
        PANIC(get_win_last_error_string());
    }
}

pipe_t input_pipe_c::get_pipe()
{
    return input_pipe();
}

void output_pipe_c::drain_message(std::string &message)
{
    buffers_mutex_.lock();
    if (process_message) {
        process_message(message, this);
    }
    else {
        for (uint i = 0; i < output_buffers.size(); ++i) {
            write_buffer(i, message.c_str(), message.size());
        }
    }
    buffers_mutex_.unlock();
}

thread_return_t output_pipe_c::drain_pipe_thread(thread_param_t param)
{
    // Can't use shared_ptr here: it causes a deadlock
    // since destructor waits for thread to finish
    output_pipe_c* self = reinterpret_cast<output_pipe_c*>(param);

    if (self->readPipe == INVALID_HANDLE_VALUE) {
        self->done_io_ = true;
        return 0;
    }

    for (uint i = 0; i < self->output_buffers.size(); ++i) {
        if (!self->output_buffers[i]->writable()) {
            self->done_io_ = true;
            return 0;
        }
    }

    for (;;)
    {
        int p0 = 0;
        char data[DEFAULT_BUFFER_SIZE + 1];
        DWORD bytes_available = 0;
        const BOOL peek_result = PeekNamedPipe(self->readPipe, NULL, 0, NULL, &bytes_available, NULL);

        if (bytes_available == 0) {
            Sleep(1);
            //self->done_io_ = true;
            if (self->stop_thread_) {
                if (!self->message_buffer.empty()) {
                    self->drain_message(self->message_buffer);
                    self->message_buffer.clear();
                }
                break;
            }
            // continue; ???
        }
        else {
            size_t bytes_count = self->read(data, DEFAULT_BUFFER_SIZE);
            if (bytes_count == 0) {
                self->done_io_ = true;
                break;
            }
            data[bytes_count] = 0;
            int p0 = self->message_buffer.size();
            self->message_buffer += data;
        }

        // TODO: store p1 value between cycle iterations
        int p1 = 0;
        do {
            p1 = 0;
            for (int i = 0; i < self->message_buffer.size(); i++) {
                if (self->message_buffer[i] == '\n') {
                    p1 = i;
                    break;
                }
            }

            if (p1 == 0
                && !self->message_buffer.empty()
                && (self->stop_thread_ || peek_result == false)) {
                p1 = self->message_buffer.length() - 1;
            }

            if (p1 > 0) {
                self->drain_message(self->message_buffer.substr(0, p1 + 1));
                self->message_buffer = self->message_buffer.substr(p1 + 1);
            }
        } while (p1 != 0);

        if (peek_result == FALSE) {
            self->done_io_ = true;
            break;
        }

    }
    self->done_io_ = true;
    return 0;
}

output_pipe_c::output_pipe_c()
    : pipe_c(PIPE_OUTPUT) {

}

output_pipe_c::~output_pipe_c() {
    wait();
}

void output_pipe_c::add_output_buffer(std::shared_ptr<output_buffer_c> output_buffer_param) {
    output_buffers.push_back(output_buffer_param);
    buffers_.push_back(output_buffer_param);
}

void output_pipe_c::bufferize()
{
    pipe_c::bufferize();
    buffer_thread = CreateThread(NULL, 0, drain_pipe_thread, this, 0, NULL);
    if (buffer_thread == NULL) {
        PANIC(get_win_last_error_string());
    }
}

pipe_t output_pipe_c::get_pipe()
{
    return output_pipe();
}

void pipe_c::remove_buffer(const std::shared_ptr<buffer_c>& buffer) {
    bool present = false;
    for (auto& b : buffers_) {
        if (b == buffer) {
            present = true;
        }
    }
    if (!present) {
        return;
    }
    if (last_buffer_ == buffer) {
        while (buffers_mutex_.is_locked()) {
            CancelSynchronousIo_wrapper(buffer_thread);
        }
        buffers_mutex_.unlock();
    }
    buffers_mutex_.lock();
    buffers_.erase(std::remove(buffers_.begin(), buffers_.end(), buffer), buffers_.end());
    remove_buffer_safe_impl_(buffer);
    buffers_mutex_.unlock();
}

void input_pipe_c::remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer) {
    input_buffers.erase(
        std::remove(input_buffers.begin(), input_buffers.end(), buffer),
        input_buffers.end());
}

void output_pipe_c::remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer) {
    output_buffers.erase(
        std::remove(output_buffers.begin(), output_buffers.end(), buffer),
        output_buffers.end());
}
