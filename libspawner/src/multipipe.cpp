#include "multipipe.h"

#include <chrono>

#include "error.h"

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

int multipipe::PIPE_ID_COUNTER = 0;
const int multipipe::SLEEP_TIME = 100;
const int multipipe::DEFAULT_BUFFER_SIZE = 65536;

multipipe::multipipe(system_pipe_ptr pipe, int bsize, pipe_mode mode, bool autostart)
    : id(++PIPE_ID_COUNTER)
    , core_pipe(pipe)
    , listen_thread(nullptr)
    , buffer_size(bsize)
    , read_tail_len(0)
    , write_tail_len(0)
    , check_new_line(true)
    , stop_flag(false)
    , mode(mode)
    , parents_count(0)
    , process_message(nullptr) {
    read_buffer = new char[buffer_size];
    read_tail_buffer = new char[buffer_size];

    if (autostart && mode == read_mode)
        start_read();
}

void multipipe::set_new_line_checking() {
    check_new_line = false;
    for (const auto& sink : sinks) {
        if (auto p = sink.second.lock()) {
            if (!p->get_pipe()->is_file() || p->parents_count > 1) {
                check_new_line = true;
            }
        }
    }
}

void multipipe::listen() {
    if (mode != read_mode && core_pipe->is_readable())
        return;

    if (process_message == nullptr) {
        process_message = [=](const char* buf, size_t count) { write(buf, count); };
    }

    while (!stop_flag) {
        auto bytes_read = core_pipe->read(read_buffer, buffer_size);
        if (bytes_read == 0) {
            break;
        }

        auto t = read_buffer;
        for (auto i = 0; i < bytes_read; i++, t++) {
            // TODO maybe increase buffer dynamic?
            read_tail_buffer[read_tail_len++] = *t;
            if (check_new_line && *t == '\n' || read_tail_len >= buffer_size) {
                process_message(read_tail_buffer, read_tail_len);
                read_tail_len = 0;
            }
        }
    }

    close_and_notify();
}

bool multipipe::stop() {
    stop_mutex.lock();
    if (listen_thread != nullptr) {
        // If necessary, the stop_flag will be set.
        system_pipe::cancel_sync_io(listen_thread->native_handle(), stop_flag);
        listen_thread->join();
        delete listen_thread;
        listen_thread = nullptr;
        stop_mutex.unlock();
        return true;
    }
    stop_mutex.unlock();
    return false;
}

void multipipe::write(const char* bytes, size_t count, set<int>& src) {
    write_mutex.lock();

    src.insert(id);
    for (const auto& pipe : sinks) {
        PANIC_IF(src.find(pipe.first) != src.end());
        if (auto p = pipe.second.lock()) {
            p->write(bytes, count, src);
        }
    }

    if (mode == write_mode)
        core_pipe->write(bytes, count);

    write_mutex.unlock();
}

void multipipe::close_and_notify() {
    flush();

    auto childs = this->sinks;
    for (const auto& pipe : childs)
        disconnect(pipe.second);

    core_pipe->close();
}

void multipipe::flush() {
    if (read_tail_len > 0) {
        write(read_tail_buffer, read_tail_len);
    }
}

multipipe_ptr multipipe::open_std(std_stream_type type, bool flush, int buffer_size) {
    return multipipe_ptr(new multipipe(system_pipe::open_std(type, flush), buffer_size, type == std_stream_input ? read_mode : write_mode));
}

multipipe_ptr multipipe::create_pipe(pipe_mode mode, bool flush, int buffer_size) {
    return multipipe_ptr(new multipipe(system_pipe::open_pipe(mode, flush), buffer_size, mode));
}

multipipe_ptr multipipe::open_file(const string& filename, bool excl, int buffer_size) {
    return multipipe_ptr(new multipipe(system_pipe::open_file(filename, read_mode, false, excl), buffer_size, read_mode, false));
}

multipipe_ptr multipipe::create_file(const string& filename, bool flush, bool excl, int buffer_size) {
    return multipipe_ptr(new multipipe(system_pipe::open_file(filename, write_mode, flush, excl), buffer_size, write_mode));
}

multipipe::~multipipe() {
    finalize();
    delete read_buffer;
    delete read_tail_buffer;
}

void multipipe::start_read() {
    if (mode != read_mode || listen_thread != nullptr)
        return;

    listen_thread = new thread(&multipipe::listen, this);
}

void multipipe::check_parents() {
    if (parents_count <= 0)
        close_and_notify();
}

void multipipe::finalize() {
    if (!stop())
        close_and_notify();
}

void multipipe::connect(weak_ptr<multipipe> pipe) {
    if (auto p = pipe.lock()) {
        sinks[p->id] = pipe;
        p->parents_count++;
    }
    set_new_line_checking();
}

void multipipe::disconnect(weak_ptr<multipipe> pipe) {
    if (auto p = pipe.lock()) {
        sinks.erase(p->id);
        p->parents_count--;
        p->check_parents();
    }
    set_new_line_checking();
}

void multipipe::write(const char* bytes, size_t count) {
    auto src = set<int>();
    write(bytes, count, src);
}

system_pipe_ptr multipipe::get_pipe() const {
    return core_pipe;
}
