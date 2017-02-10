#include "buffer.h"

#include <cmath>
#include <cstdio>

#include "error.h"
#include "pipes.h"

duplex_buffer_c::duplex_buffer_c() {
    if (!CreatePipe(&in, &out, NULL, 0)) {
        PANIC(get_win_last_error_string());
    }
}

bool duplex_buffer_c::readable() {
    return true;
}

bool duplex_buffer_c::writable() {
    return true;
}

size_t duplex_buffer_c::read(void *data, size_t size) {
    DWORD bytes_read;
    ReadFile(in, data, size, &bytes_read, NULL);
    return bytes_read;
}

size_t duplex_buffer_c::write_impl_(const void *data, size_t size) {
    DWORD bytes_written;
    WriteFile(out, data, size, &bytes_written, NULL);
    FlushFileBuffers(out);
    return bytes_written;
}

int duplex_buffer_c::peek() const {
    DWORD bytes_available = 0;
    const BOOL peek_result = PeekNamedPipe(in, NULL, 0, NULL, &bytes_available, NULL);
    return bytes_available;
}

handle_buffer_c::handle_buffer_c()
    : stream(handle_default_value) {
}

size_t handle_buffer_c::protected_read(void *data, size_t size) {
    DWORD bytes_read = 0;
    ReadFile(stream, data, size, &bytes_read, NULL);
    return bytes_read;
}

size_t handle_buffer_c::protected_write(const void *data, size_t size) {
    DWORD bytes_written = 0;
    if (!WriteFile(stream, data, size, &bytes_written, NULL))
        PANIC(get_win_last_error_string());
    return bytes_written;
}

void handle_buffer_c::init_handle(handle_t stream_arg) {
    stream = stream_arg;
}

handle_buffer_c::~handle_buffer_c() {
    if (!dont_close_handle_) {
        CloseHandleSafe(stream);
    }
}

input_file_buffer_c::input_file_buffer_c() {
}

input_file_buffer_c::input_file_buffer_c(const std::string &file_name) {
    handle_t handle = CreateFile(file_name.c_str(), GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        PANIC(file_name + ": " + get_win_last_error_string());
    }
    init_handle(handle);
}

bool input_file_buffer_c::readable() {
    return (stream != handle_default_value);
}

size_t input_file_buffer_c::read(void *data, size_t size) {
    return protected_read(data, size);
}

output_file_buffer_c::output_file_buffer_c() {
}

output_file_buffer_c::output_file_buffer_c(const std::string &file_name) {
    handle_t handle = CreateFile(file_name.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        PANIC(file_name + ": " + get_win_last_error_string());
    }
    init_handle(handle);
}

bool output_file_buffer_c::writable() {
    return (stream != handle_default_value);
}

size_t output_file_buffer_c::write_impl_(const void *data, size_t size) {
    return protected_write(data, size);
}

mutex_c output_stdout_buffer_c::stdout_write_mutex_;

output_stdout_buffer_c::output_stdout_buffer_c(const unsigned int &color_param)
    : color(color_param) {

    dont_close_handle_ = true;
    handle_t handle = GetStdHandle(STD_OUTPUT_HANDLE);
    init_handle(handle);
}

output_stdout_buffer_c::~output_stdout_buffer_c() {
}

bool output_stdout_buffer_c::writable() {
    return (stream != handle_default_value);
}

size_t output_stdout_buffer_c::write_impl_(const void *data, size_t size) {
    size_t result = 0;
    stdout_write_mutex_.lock();
    if (color) {
        WORD attributes = color;
        if (!SetConsoleTextAttribute(stream, attributes))
            PANIC(get_win_last_error_string());
    }
    result = protected_write(data, size);
    if (color) {
        WORD attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        if (!SetConsoleTextAttribute(stream, attributes))
            PANIC(get_win_last_error_string());
    }
    fflush(stdout);
    stdout_write_mutex_.unlock();
    return result;
}

input_stdin_buffer_c::input_stdin_buffer_c() {
    dont_close_handle_ = true;
    handle_t handle = GetStdHandle(STD_INPUT_HANDLE);
    init_handle(handle);
}

bool input_stdin_buffer_c::readable() {
    return (stream != handle_default_value);
}

size_t input_stdin_buffer_c::read(void *data, size_t size) {
    size_t result = protected_read(data, size);

    return result;
}

size_t pipe_buffer_c::write_impl_(const void *data, size_t size) {
    return pipe_->write(data, size);
}

pipe_buffer_c::pipe_buffer_c(const std::shared_ptr<input_pipe_c>& pipe)
    : pipe_(pipe) {

}
