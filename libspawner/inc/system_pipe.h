#ifndef SYSTEM_PIPE
#define SYSTEM_PIPE

#include <memory>

#include "platform.h"

using std::shared_ptr;
using std::string;

class system_pipe;
typedef shared_ptr<system_pipe> system_pipe_ptr;

enum std_stream_type
{
    std_stream_input,
    std_stream_output,
    std_stream_error
};

enum pipe_mode
{
    read_mode = 1,
    write_mode = 2,
};

class system_pipe {
    bool file_flag;

    pipe_handle input_handle;
    pipe_handle output_handle;

    explicit system_pipe(bool is_file);

public:
    static system_pipe_ptr open_std(std_stream_type type);
    static system_pipe_ptr open_pipe(pipe_mode mode);
    static system_pipe_ptr open_file(const string& filename, pipe_mode mode);

    pipe_handle get_input_handle() const;
    pipe_handle get_output_handle() const;

    ~system_pipe();

    bool is_readable() const;
    bool is_writable() const;
    size_t read(char* bytes, size_t count) const;
    size_t write(const char* bytes, size_t count) const;
    void flush() const;
    void close(pipe_mode mode);
    void close();

    bool is_file() const;

    static void cancel_sync_io(thread_t thread);
};

#endif // SYSTEM_PIPE
