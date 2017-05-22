#include "system_pipe.h"

#include <Windows.h>

#include "error.h"

system_pipe::system_pipe(bool flush, pipe_type t) {
    autoflush = flush;
    type = t;
    input_handle = INVALID_HANDLE_VALUE;
    output_handle = INVALID_HANDLE_VALUE;
}

system_pipe_ptr system_pipe::open_std(std_stream_type type, bool flush) {
    auto pipe = new system_pipe(flush, pipe_type::con);

    switch (type) {
        case std_stream_input:
            pipe->input_handle = GetStdHandle(STD_INPUT_HANDLE);
            break;
        case std_stream_output:
            pipe->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            break;
        case std_stream_error:
            pipe->output_handle = GetStdHandle(STD_ERROR_HANDLE);
            break;
        default:
            PANIC("Bad pipe mode");
    }

    return system_pipe_ptr(pipe);
}

system_pipe_ptr system_pipe::open_pipe(pipe_mode mode, bool flush) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    auto pipe = new system_pipe(flush);

    if (!CreatePipe(&pipe->input_handle, &pipe->output_handle, &saAttr, 0))
        PANIC(get_win_last_error_string());

    if (mode == write_mode && !SetHandleInformation(pipe->output_handle, HANDLE_FLAG_INHERIT, 0))
        PANIC(get_win_last_error_string());

    if (mode == read_mode && !SetHandleInformation(pipe->input_handle, HANDLE_FLAG_INHERIT, 0))
        PANIC(get_win_last_error_string());

    return system_pipe_ptr(pipe);
}

system_pipe_ptr system_pipe::open_file(const string& filename, pipe_mode mode, bool flush, bool excl) {
    DWORD access;
    DWORD creationDisposition;
    if (mode == read_mode) {
        access = GENERIC_READ;
        creationDisposition = OPEN_EXISTING;
    } else if (mode == write_mode) {
        access = GENERIC_WRITE;
        creationDisposition = CREATE_ALWAYS;
    } else
        PANIC("Bad pipe mode");

    auto file = CreateFile(filename.c_str(), access, excl ? 0 : FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        PANIC(filename + ": " + get_win_last_error_string());

    auto pipe = new system_pipe(flush, pipe_type::file);

    if (mode == read_mode)
        pipe->input_handle = file;
    if (mode == write_mode)
        pipe->output_handle = file;

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
    return input_handle != INVALID_HANDLE_VALUE;
}

bool system_pipe::is_writable() const {
    return output_handle != INVALID_HANDLE_VALUE;
}

size_t system_pipe::read(char* bytes, size_t count) {
    size_t bytes_read = 0;

    read_mutex.lock();
    if (is_readable() && !ReadFile(input_handle, (LPVOID)bytes, (DWORD)count, (LPDWORD)&bytes_read, nullptr)) {
        auto error = GetLastError();
        // ReadFile may be canceled by CancelSynchronousIo.
        // Pipe may be already closed by application.
        if (error != ERROR_OPERATION_ABORTED && error != ERROR_BROKEN_PIPE) {
            read_mutex.unlock();
            PANIC(get_win_last_error_string());
        }
    }
    read_mutex.unlock();

    return bytes_read;
}

size_t system_pipe::write(const char* bytes, size_t count) {
    size_t bytes_written = 0;

    write_mutex.lock();
    if (is_writable() && !WriteFile(output_handle, (LPCVOID)bytes, (DWORD)count, (LPDWORD)&bytes_written, nullptr)) {
        auto error = GetLastError();
        // Pipe may be already closed by application.
        if (error != ERROR_BROKEN_PIPE) {
            write_mutex.unlock();
            PANIC(get_win_last_error_string());
        }
    }
    write_mutex.unlock();

    if (bytes_written > 0 && autoflush)
        flush();

    return bytes_written;
}

void system_pipe::flush() {
    write_mutex.lock();
    if (is_writable()) {
        // If the child process exits before reading all data from  the pipe, FlushFileBuffers will hang.
        // To work around this, we close the pipe handle and ignore errors in write() function.
        FlushFileBuffers(output_handle);
    }
    write_mutex.unlock();
}

void system_pipe::close(pipe_mode mode) {
    if (mode == read_mode && is_readable()) {
        read_mutex.lock();
        CloseHandle(input_handle);
        input_handle = INVALID_HANDLE_VALUE;
        read_mutex.unlock();
    }

    if (mode == write_mode && is_writable()) {
        flush();
        write_mutex.lock();
        CloseHandle(output_handle);
        output_handle = INVALID_HANDLE_VALUE;
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
    stop = true;
    if (!CancelSynchronousIo(thread) && GetLastError() != ERROR_NOT_FOUND) {
        PANIC(get_win_last_error_string());
    }
}
