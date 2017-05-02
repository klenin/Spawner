#include <Windows.h>

#include "system_pipe.h"

system_pipe::system_pipe(bool is_file) {
    file_flag = is_file;
    input_handle = INVALID_HANDLE_VALUE;
    output_handle = INVALID_HANDLE_VALUE;
}

system_pipe_ptr system_pipe::open_std(std_stream_type type) {
    auto pipe = new system_pipe(true); // true -> fix for Ctrl+Z(win)|Ctrl+D(posix)

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

system_pipe_ptr system_pipe::open_pipe(pipe_mode mode) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    auto pipe = new system_pipe(false);

    if (!CreatePipe(&pipe->input_handle, &pipe->output_handle, &saAttr, 0))
        PANIC(get_win_last_error_string());

    if (mode == write_mode && !SetHandleInformation(pipe->output_handle, HANDLE_FLAG_INHERIT, 0))
        PANIC(get_win_last_error_string());

    if (mode == read_mode && !SetHandleInformation(pipe->input_handle, HANDLE_FLAG_INHERIT, 0))
        PANIC(get_win_last_error_string());

    return system_pipe_ptr(pipe);
}

system_pipe_ptr system_pipe::open_file(const string& filename, pipe_mode mode) {
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

    auto file = CreateFile(filename.c_str(), access, FILE_SHARE_READ, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        PANIC(filename + ": " + get_win_last_error_string());

    auto pipe = new system_pipe(true);

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

size_t system_pipe::read(char* bytes, size_t count) const {
    if (!is_readable())
        return 0;

    size_t bytes_read;
    if (!ReadFile(input_handle, (LPVOID)bytes, (DWORD)count, (LPDWORD)&bytes_read, nullptr)) {
        auto error = GetLastError();
        // We force closed write side of the pipe.
        if (error != ERROR_OPERATION_ABORTED && error != ERROR_BROKEN_PIPE)
            PANIC(get_win_last_error_string());
    }

    return bytes_read;
}

size_t system_pipe::write(const char* bytes, size_t count) const {
    if (!is_writable())
        return 0;

    size_t bytes_written;
    if (!WriteFile(output_handle, (LPCVOID)bytes, (DWORD)count, (LPDWORD)&bytes_written, nullptr)) {
        auto error = GetLastError();
        // Pipe may be already closed.
        if (error != ERROR_OPERATION_ABORTED && error != ERROR_BROKEN_PIPE)
            PANIC(get_win_last_error_string());
    }

    if (bytes_written > 0)
        flush();

    return bytes_written;
}

void system_pipe::flush() const {
    if (!is_writable())
        return;

    FlushFileBuffers(output_handle);
}

void system_pipe::close(pipe_mode mode) {
    if (mode == read_mode && is_readable()) {
        CloseHandle(input_handle);
        input_handle = INVALID_HANDLE_VALUE;
    }

    if (mode == write_mode && is_writable()) {
        CloseHandle(output_handle);
        output_handle = INVALID_HANDLE_VALUE;
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
    if(!CancelSynchronousIo(thread) && GetLastError() != ERROR_NOT_FOUND) {
        PANIC(get_win_last_error_string());
    }
}
