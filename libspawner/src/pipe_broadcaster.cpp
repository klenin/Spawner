#include <chrono>

#include "pipe_broadcaster.h"

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

int pipe_broadcaster::PIPE_ID_COUNTER = 0;
const int pipe_broadcaster::SLEEP_TIME = 100;
const int pipe_broadcaster::DEFAULT_BUFFER_SIZE = 65536;

pipe_broadcaster::pipe_broadcaster(pipe_ptr pipe, int bsize, pipe_mode mode, bool autostart)
    : id(++PIPE_ID_COUNTER)
    , core_pipe(pipe)
    , listen_thread(nullptr)
    , done(false)
    , buffer_size(bsize)
    , read_tail_len(0)
    , write_tail_len(0)
    , mode(mode)
    , process_message(nullptr) {
    read_buffer = new char[buffer_size];
    read_tail_buffer = new char[buffer_size];

    if (autostart && mode == read_mode)
        start_read();
}

void pipe_broadcaster::listen() {
    if (mode != read_mode && core_pipe->is_readable())
        return;

    if (process_message == nullptr) {
        process_message = [=](const char* buf, size_t count) { write(buf, count); };
    }

    while (!done) {
        auto readed = core_pipe->read(read_buffer, buffer_size);
        if (readed == 0) {
            if (core_pipe->is_file() && core_pipe->is_readable()) {
                break;
            }
            sleep_for(milliseconds(SLEEP_TIME));
            continue;
        }

        auto t = read_buffer;
        for (auto i = 0; i < readed; i++, t++) {
            // TODO maybe increase buffer dynamic?
            read_tail_buffer[read_tail_len++] = *t;
            if (*t == '\n' || read_tail_len >= buffer_size) {
                process_message(read_tail_buffer, read_tail_len);
                read_tail_len = 0;
            }
        }
    }

    close_and_notify();
}

bool pipe_broadcaster::stop() {
    if (listen_thread != nullptr) {
        done = true;
        system_pipe::cancel_sync_io(listen_thread->native_handle());
        listen_thread->join();
        delete listen_thread;
        listen_thread = nullptr;
        return true;
    }
    return false;
}

void pipe_broadcaster::write(const char* bytes, size_t count, set<int>& src) {
    write_mutex.lock();

    src.insert(id);
    for (auto& pipe : childs) {
        PANIC_IF(src.find(pipe.first) != src.end());
        if (auto p = pipe.second.lock()) {
            p->write(bytes, count, src);
        }
    }

    if (mode == write_mode)
        core_pipe->write(bytes, count);

    write_mutex.unlock();
}

void pipe_broadcaster::close_and_notify() {
    flush();

    auto childs = this->childs;
    for (auto& pipe : childs)
        disconnect(pipe.second);

    core_pipe->close();
}

void pipe_broadcaster::flush() {
    if (read_tail_len > 0) {
        write(read_tail_buffer, read_tail_len);
    }
}

pipe_broadcaster_ptr pipe_broadcaster::open_std(std_stream_type type, int buffer_size) {
    return pipe_broadcaster_ptr(new pipe_broadcaster(system_pipe::open_std(type), buffer_size, type == std_stream_input ? read_mode : write_mode));
}

pipe_broadcaster_ptr pipe_broadcaster::create_pipe(pipe_mode mode, int buffer_size) {
    return pipe_broadcaster_ptr(new pipe_broadcaster(system_pipe::open_pipe(mode), buffer_size, mode));
}

pipe_broadcaster_ptr pipe_broadcaster::open_file(const string& filename, int buffer_size) {
    return pipe_broadcaster_ptr(new pipe_broadcaster(system_pipe::open_file(filename, read_mode), buffer_size, read_mode, false));
}

pipe_broadcaster_ptr pipe_broadcaster::create_file(const string& filename, int buffer_size) {
    return pipe_broadcaster_ptr(new pipe_broadcaster(system_pipe::open_file(filename, write_mode), buffer_size, write_mode));
}

pipe_broadcaster::~pipe_broadcaster() {
    if (!stop())
        close_and_notify();
    delete read_buffer;
    delete read_tail_buffer;
}

void pipe_broadcaster::start_read() {
    if (mode != read_mode || listen_thread != nullptr)
        return;

    listen_thread = new thread(&pipe_broadcaster::listen, this);
}

void pipe_broadcaster::check_parents() {
    if (parents.size() == 0)
        close_and_notify();
}

void pipe_broadcaster::connect(weak_ptr<pipe_broadcaster> pipe) {
    if (auto p = pipe.lock()) {
        childs[p->id] = pipe;
        p->parents.insert(id);
    }
}

void pipe_broadcaster::disconnect(weak_ptr<pipe_broadcaster> pipe) {
    if (auto p = pipe.lock()) {
        childs.erase(p->id);
        p->parents.erase(id);
        p->check_parents();
    }
}

void pipe_broadcaster::write(const char* bytes, size_t count) {
    auto src = set<int>();
    write(bytes, count, src);
}

pipe_ptr pipe_broadcaster::get_pipe() const {
    return core_pipe;
}
