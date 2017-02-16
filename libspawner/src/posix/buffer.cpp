#include "buffer.hpp"

#include <cmath>
#include <cstdio>

#include <unistd.h>

#include "error.hpp"
#include "pipes.hpp"

duplex_buffer_c::duplex_buffer_c() {
}

bool duplex_buffer_c::readable() {
    return true;
}

bool duplex_buffer_c::writable() {
    return true;
}

size_t duplex_buffer_c::read(void *data, size_t size) {
    size_t bytes_read;
    return bytes_read;
}

size_t duplex_buffer_c::write_impl_(const void *data, size_t size) {
    size_t bytes_written;
    fsync(out);
    return bytes_written;
}

int duplex_buffer_c::peek() const {
    int bytes_available = 0;
    return bytes_available;
}

handle_buffer_c::handle_buffer_c()
    : stream(handle_default_value) {
}

size_t handle_buffer_c::protected_read(void *data, size_t size) {
    size_t bytes_read = 0;
    return bytes_read;
}

size_t handle_buffer_c::protected_write(const void *data, size_t size) {
    size_t bytes_written = 0;
    return bytes_written;
}

void handle_buffer_c::init_handle(handle_t stream_arg) {
    stream = stream_arg;
}

handle_buffer_c::~handle_buffer_c() {
    if (!dont_close_handle_) {
    }
}

input_file_buffer_c::input_file_buffer_c() {
}

input_file_buffer_c::input_file_buffer_c(const std::string &file_name) {
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
}

output_stdout_buffer_c::~output_stdout_buffer_c() {
}

bool output_stdout_buffer_c::writable() {
    return (stream != handle_default_value);
}

size_t output_stdout_buffer_c::write_impl_(const void *data, size_t size) {
    size_t result = 0;
    stdout_write_mutex_.lock();
    //fflush(stdout);
    stdout_write_mutex_.unlock();
    return result;
}

input_stdin_buffer_c::input_stdin_buffer_c() {
    dont_close_handle_ = true;
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
