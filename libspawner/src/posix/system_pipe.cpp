#include <error.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "system_pipe.h"

system_pipe::system_pipe(bool is_file) {
    file_flag = is_file;
    input_handle = -1;
    output_handle = -1;
}

pipe_ptr system_pipe::open_std(std_stream_type type) {
    auto pipe = new system_pipe(true); // true -> fix for Ctrl+Z(win)|Ctrl+D(posix)

    switch (type) {
        case std_stream_input:
            pipe->input_handle = STDIN_FILENO;
            break;
        case std_stream_output:
            pipe->output_handle = STDOUT_FILENO;
            break;
        case std_stream_error:
            pipe->output_handle = STDOUT_FILENO;
            break;
        default:
            PANIC("Unknown std stream type");
    }

    return pipe_ptr(pipe);
}

pipe_ptr system_pipe::open_pipe(pipe_mode mode) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        PANIC(strerror(errno));
    }

    auto pipe = new system_pipe(false);
    pipe->input_handle = pipefd[0];
    pipe->output_handle = pipefd[1];
    return pipe_ptr(pipe);
}

pipe_ptr system_pipe::open_file(const string& filename, pipe_mode mode) {
    auto oflag = 0;
    if (mode == read_mode) {
        oflag |= O_RDONLY | O_NOFOLLOW;
    } else if (mode == write_mode) {
        oflag |= O_WRONLY | O_CREAT | O_NOFOLLOW,
                S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;
    } else
        PANIC("Bad pipe mode");

    int fd;
    if ((fd = open(filename.c_str(), oflag)) < 0) {
        PANIC(filename + ": " + strerror(errno));
    }

    auto pipe = new system_pipe(true);

    if (mode == read_mode)
        pipe->input_handle = fd;
    if (mode == write_mode)
        pipe->output_handle = fd;

    return pipe_ptr(pipe);
}

pipe_handle system_pipe::get_input_handle() const {
    return input_handle;
}

pipe_handle system_pipe::get_output_handle() const {
    return output_handle;
}

system_pipe::~system_pipe() {
    close();
}

bool system_pipe::is_readable() const {
    return input_handle >= 0;
}

bool system_pipe::is_writable() const {
    return output_handle >= 0;
}

size_t system_pipe::read(char* bytes, size_t count) const {
    if (!is_readable())
        return 0;

    ssize_t readed = ::read(input_handle, bytes, count);
    if (readed < 0)
        PANIC(strerror(errno));

    return (size_t)readed;
}

size_t system_pipe::write(const char* bytes, size_t count) const {
    if (!is_writable())
        return 0;

    ssize_t writed = ::write(output_handle, bytes, count);
    if (writed < 0)
        PANIC(strerror(errno));
    if (writed > 0)
        flush();

    return (size_t)writed;
}

void system_pipe::flush() const {
    if (!is_writable())
        return;

    fsync(output_handle);
}

void system_pipe::close(pipe_mode mode) {
    if (mode == read_mode && is_readable()) {
        ::close(input_handle);
        input_handle = -1;
    }

    if (mode == write_mode && is_writable()) {
        ::close(output_handle);
        output_handle = -1;
    }
}

void system_pipe::close() {
    close(read_mode);
    close(write_mode);
}

bool system_pipe::is_file() const {
    return file_flag;
}

void system_pipe::cancel_sync_io(thread_t thread) {
    // TODO research how to (phtread_sigmask)
}
