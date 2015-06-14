#include "buffer.h"

#include <memory>

#include "error.h"
#include "pipes.h"

duplex_buffer_c::duplex_buffer_c() {
    if (!CreatePipe(&in, &out, NULL, 0)) {
        //raise_error(*this, "CreatePipe");
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
    WriteFile(stream, data, size, &bytes_written, NULL);
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
        PANIC(get_win_last_error_string());
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
        PANIC(get_win_last_error_string());
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
        {
            //    throw GetWin32Error("SetConsoleTextAttribute");
        }
    }
    result = protected_write(data, size);
    if (color) {
        WORD attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
        if (!SetConsoleTextAttribute(stream, attributes))
        {
            //    throw GetWin32Error("SetConsoleTextAttribute");
        }
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

void dprintf(const char* format, ...) {
  int final_n;
  int n = strlen(format) * 2;
  std::string str;
  std::unique_ptr<char[]> formatted;
  va_list ap;
  for (;;) {
    formatted.reset(new char[n]);
    strcpy(&formatted[0], format);
    va_start(ap, format);
    final_n = vsnprintf(&formatted[0], n, format, ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n) {
      n += std::abs(final_n - n + 1);
    } else {
      break;
    }
  }
  static output_stdout_buffer_c stdout_buffer(FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED);
  stdout_buffer.write(formatted.get(), final_n);
}

size_t pipe_buffer_c::write_impl_(const void *data, size_t size) {
    return pipe_->write(data, size);
}

pipe_buffer_c::pipe_buffer_c(const std::shared_ptr<input_pipe_c>& pipe)
    : pipe_(pipe) {

}
