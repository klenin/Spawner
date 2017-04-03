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

class pipe_broadcaster;
typedef shared_ptr<pipe_broadcaster> pipe_broadcaster_ptr;

class pipe_broadcaster {
    static int PIPE_ID_COUNTER;
    static const int SLEEP_TIME;
    static const int DEFAULT_BUFFER_SIZE;

    int id;

    pipe_ptr core_pipe;
    thread* listen_thread;
    mutex write_mutex;

    bool done;
    int buffer_size;
    char *read_buffer, *read_tail_buffer;
    size_t read_tail_len, write_tail_len;

    pipe_mode mode;
    set<int> parents;
    map<int, weak_ptr<pipe_broadcaster>> childs;

    pipe_broadcaster(pipe_ptr pipe, int buffer_size, pipe_mode mode, bool autostart = true);

    void listen();
    bool stop();

    void write(const char* bytes, size_t count, set<int>& src);

    void close_and_notify();
    void flush();

public:
    static pipe_broadcaster_ptr open_std(std_stream_type type, int buffer_size = DEFAULT_BUFFER_SIZE);
    static pipe_broadcaster_ptr create_pipe(pipe_mode mode, int buffer_size = DEFAULT_BUFFER_SIZE);
    static pipe_broadcaster_ptr open_file(const string& filename, int buffer_size = DEFAULT_BUFFER_SIZE);
    static pipe_broadcaster_ptr create_file(const string& filename, int buffer_size = DEFAULT_BUFFER_SIZE);
    ~pipe_broadcaster();

    void start_read();

    void connect(weak_ptr<pipe_broadcaster> pipe);
    void disconnect(weak_ptr<pipe_broadcaster> pipe);

    void write(const char* bytes, size_t count);

    pipe_ptr get_pipe() const;

    std::function<void(const char* buffer, size_t count)> process_message;
};

#endif // PIPE_H
