#ifndef SYSTEM_PIPE
#define SYSTEM_PIPE

#include <memory>
#include <mutex>

#include "platform.h"

using std::mutex;
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
    enum pipe_type
    {
        def  = 0,
        file = 1,
        con  = 2,
    };
    pipe_type type;

    pipe_handle input_handle;
    pipe_handle output_handle;

    mutex read_mutex, write_mutex;

    bool autoflush;

    explicit system_pipe(bool flush, pipe_type t = pipe_type::def);

public:
    static system_pipe_ptr open_std(std_stream_type type, bool flush = true);
    static system_pipe_ptr open_pipe(pipe_mode mode, bool flush = true);
    static system_pipe_ptr open_file(const string& filename, pipe_mode mode, bool flush = false, bool excl = false);

    pipe_handle get_input_handle() const;
    pipe_handle get_output_handle() const;

    ~system_pipe();

    bool is_readable() const;
    bool is_writable() const;
    size_t read(char* bytes, size_t count);
    size_t write(const char* bytes, size_t count);
    void flush();
    void close(pipe_mode mode);
    void close();

    bool is_file() const;
    bool is_console() const;

    static void cancel_sync_io(thread_t thread, bool &stop);
};

#endif // SYSTEM_PIPE
