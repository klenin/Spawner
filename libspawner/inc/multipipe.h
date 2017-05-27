#ifndef PIPE_H
#define PIPE_H

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include "system_pipe.h"

using std::mutex;
using std::shared_ptr;
using std::string;
using std::thread;
using std::map;
using std::set;
using std::weak_ptr;

class multipipe;
typedef shared_ptr<multipipe> multipipe_ptr;

class multipipe {
    static int PIPE_ID_COUNTER;
    static const int SLEEP_TIME;
    static const int DEFAULT_BUFFER_SIZE;

    int id;

    system_pipe_ptr core_pipe;
    thread* listen_thread;
    mutex write_mutex, stop_mutex;

    int buffer_size;
    char *read_buffer, *read_tail_buffer;
    size_t read_tail_len, write_tail_len;
    bool check_new_line, custom_process_message, stop_flag;

    pipe_mode mode;
    volatile int parents_count;
    map<int, weak_ptr<multipipe>> sinks;

    std::function<void(const char* buffer, size_t count)> process_message;

    multipipe(system_pipe_ptr pipe, int buffer_size, pipe_mode mode, bool autostart = true);

    void set_new_line_checking();
    void listen();
    bool stop();

    void write(const char* bytes, size_t count, set<int>& src);

    void close_and_notify();
    void flush();

public:
    static multipipe_ptr open_std(std_stream_type type, bool flush = true, int buffer_size = DEFAULT_BUFFER_SIZE);
    static multipipe_ptr create_pipe(pipe_mode mode, bool flush = true, int buffer_size = DEFAULT_BUFFER_SIZE);
    static multipipe_ptr open_file(const string& filename, bool excl = false, int buffer_size = DEFAULT_BUFFER_SIZE);
    static multipipe_ptr create_file(const string& filename, bool flush = false, bool excl = false, int buffer_size = DEFAULT_BUFFER_SIZE);
    ~multipipe();

    void start_read();
    void check_parents();
    void finalize();

    void connect(weak_ptr<multipipe> pipe);
    void disconnect(weak_ptr<multipipe> pipe);
    void for_each_sink(std::function<void(multipipe_ptr& sink)> func);

    void write(const char* bytes, size_t count);

    void set_custom_process_message(std::function<void(const char* buffer, size_t count)> func);
    bool process_message_is_custom() const;

    system_pipe_ptr get_pipe() const;
};

#endif // PIPE_H
