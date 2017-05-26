#include "system_pipe.h"

#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

#include "error.h"

system_pipe::system_pipe(bool flush, pipe_type t) {
    autoflush = flush;
    type = t;
    input_handle = -1;
    output_handle = -1;
}

system_pipe_ptr system_pipe::open_std(std_stream_type type, bool flush) {
    auto pipe = new system_pipe(flush, pipe_type::con);

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

    return system_pipe_ptr(pipe);
}

system_pipe_ptr system_pipe::open_pipe(pipe_mode mode, bool flush) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        PANIC(strerror(errno));
    }

    auto pipe = new system_pipe(flush);
    pipe->input_handle = pipefd[0];
    pipe->output_handle = pipefd[1];
    return system_pipe_ptr(pipe);
}

system_pipe_ptr system_pipe::open_file(const string& filename, pipe_mode mode, bool flush, bool excl) {
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
    if (excl) {
        flock(fd, LOCK_EX);
    }

    auto pipe = new system_pipe(flush, pipe_type::file);

    if (mode == read_mode)
        pipe->input_handle = fd;
    if (mode == write_mode)
        pipe->output_handle = fd;

    return system_pipe_ptr(pipe);
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

size_t system_pipe::read(char* bytes, size_t count) {
    ssize_t bytes_read = 0;

    if (is_readable()) {
        read_mutex.lock();
        bytes_read = ::read(input_handle, bytes, count);
        if (bytes_read < 0) {
            read_mutex.unlock();
            PANIC(strerror(errno));
        }
        read_mutex.unlock();
    }

    return (size_t)bytes_read;
}

size_t system_pipe::write(const char* bytes, size_t count) {
    ssize_t bytes_written = 0;

    if (is_writable()) {
        write_mutex.lock();
        bytes_written = ::write(output_handle, bytes, count);
        if (bytes_written < 0 && errno != EPIPE && errno != EBADF) {
            write_mutex.unlock();
            PANIC(strerror(errno));
        }
        write_mutex.unlock();
    }

    if (bytes_written > 0 && autoflush)
        flush();

    return (size_t)bytes_written;
}

void system_pipe::flush() {
    write_mutex.lock();
    if (is_writable())
        fsync(output_handle);
    write_mutex.unlock();
}

void system_pipe::close(pipe_mode mode) {
    if (mode == read_mode && is_readable()) {
        read_mutex.lock();
        if (is_file()) {
            flock(input_handle, LOCK_UN);
        }
        ::close(input_handle);
        input_handle = -1;
        read_mutex.unlock();
    }

    if (mode == write_mode && is_writable()) {
        flush();
        write_mutex.lock();
        if (is_file()) {
            flock(output_handle, LOCK_UN);
        }
        ::close(output_handle);
        output_handle = -1;
        write_mutex.unlock();
    }
}

void system_pipe::close() {
    close(read_mode);
    close(write_mode);
}

bool system_pipe::is_file() const {
    return type == file;
}

bool system_pipe::is_console() const {
    return type == con;
}

void system_pipe::cancel_sync_io(thread_t thread, bool &stop) {
    // Do nothing, because read() is unlocked when the pipe closes.
}
