#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdarg>
#include <memory>

#include "inc/platform.h"
#include "inc/mutex.h"

class buffer_c {
public:
    virtual ~buffer_c() {

    }
};

class input_buffer_c: public virtual buffer_c {
public:
    virtual bool readable() = 0;
    virtual size_t read(void *data, size_t size) = 0;
};

class output_buffer_c: public virtual buffer_c {
public:
    output_buffer_c() {
        write_mutex_.possess();
    }
    virtual ~output_buffer_c() {
        write_mutex_.release();
    }
    virtual bool writable() = 0;
    size_t write(const void *data, size_t size) {
        write_mutex_.lock();
        size_t r = write_impl_(data, size);
        write_mutex_.unlock();
        return r;
    }

private:
    mutex_c write_mutex_;
    virtual size_t write_impl_(const void *data, size_t size) = 0;
};

class duplex_buffer_c
    : public input_buffer_c
    , public output_buffer_c {
protected:
    handle_t in;
    handle_t out;

public:
    duplex_buffer_c();
    virtual bool readable();
    virtual bool writable();
    virtual size_t read(void *data, size_t size);
    virtual size_t write_impl_(const void *data, size_t size);
};

class handle_buffer_c {
protected:
    bool dont_close_handle_ = false;
    handle_t stream;
    size_t protected_read(void *data, size_t size);
    size_t protected_write(const void *data, size_t size);
    void init_handle(handle_t stream_arg);

public:
    handle_buffer_c();
    virtual ~handle_buffer_c();
};

class input_file_buffer_c
    : public input_buffer_c
    , protected handle_buffer_c {
public:
    input_file_buffer_c();
    input_file_buffer_c(const std::string &file_name);
    virtual bool readable();
    virtual size_t read(void *data, size_t size);
};

class output_file_buffer_c
    : public output_buffer_c
    , protected handle_buffer_c {
public:
    output_file_buffer_c();
    output_file_buffer_c(const std::string &file_name);
    virtual bool writable();

private:
    virtual size_t write_impl_(const void *data, size_t size);
};

class output_stdout_buffer_c
    : public output_buffer_c
    , protected handle_buffer_c {
protected:
    unsigned color;

public:
    output_stdout_buffer_c() = delete;
    output_stdout_buffer_c(const unsigned &color_param = 0);
    virtual ~output_stdout_buffer_c();
    virtual bool writable();

private:
    static mutex_c stdout_write_mutex_;
    virtual size_t write_impl_(const void *data, size_t size);
};

void dprintf(const char* format, ...);

class input_stdin_buffer_c
    : public input_buffer_c
    , protected handle_buffer_c {
protected:
    unsigned color;

public:
    input_stdin_buffer_c();
    virtual bool readable();
    virtual size_t read(void *data, size_t size);
};

class input_pipe_c;
class pipe_buffer_c : public output_buffer_c {
public:
    pipe_buffer_c(const std::shared_ptr<input_pipe_c>& pipe);
    virtual bool writable() {
        return true;
    }
private:
    std::shared_ptr<input_pipe_c> pipe_;
    virtual size_t write_impl_(const void *data, size_t size);
};
