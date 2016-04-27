#pragma once

#include <sstream>
#include <string>
#include <memory>
#include <functional>
#include <atomic>

#include "inc/platform.h"
#include "inc/buffer.h"
#include "inc/session.h"
#include "inc/mutex.h"

enum pipes_t/*rename*/ {
    STD_INPUT_PIPE,
    STD_OUTPUT_PIPE,
    STD_ERROR_PIPE
};

enum std_pipe_t {
    PIPE_INPUT,
    PIPE_OUTPUT,

    PIPE_UNDEFINED,
};

class pipe_c
{
private:
    mutex_c write_mutex;
protected:
    void create_pipe();
    handle_t buffer_thread;
    pipe_t readPipe;
    pipe_t writePipe;
    std_pipe_t pipe_type;
    bool state;
    pipe_t input_pipe();
    pipe_t output_pipe();
    void wait();
    std::atomic<bool> stop_thread_{false};
    std::atomic<bool> done_io_{false};
    mutex_c buffers_mutex_;
    std::shared_ptr<buffer_c> last_buffer_;
    std::vector<std::shared_ptr<buffer_c>> buffers_;
    virtual void remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer) = 0;
public:
    pipe_c();
    pipe_c(const std_pipe_t &pipe_type);
    void close_pipe();
    virtual ~pipe_c();
    size_t write(const void *data, size_t size);
    size_t read(void *data, size_t size);
    virtual void bufferize();
    void finish();
    bool valid();
    virtual pipe_t get_pipe();
    void remove_buffer(const std::shared_ptr<buffer_c>& buffer);
};

class input_pipe_c: public pipe_c
{
protected:
    static thread_return_t fill_pipe_thread(thread_param_t param);
    std::vector<std::shared_ptr<input_buffer_c>> input_buffers;
    virtual void remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer);
public:
    input_pipe_c();
    virtual ~input_pipe_c();
    virtual void add_input_buffer(std::shared_ptr<input_buffer_c> input_buffer_param);
    virtual void bufferize();
    virtual pipe_t get_pipe();
};

class output_pipe_c: public pipe_c
{
protected:
    static thread_return_t drain_pipe_thread(thread_param_t param);
    std::string message_buffer;
    void remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer);
    std::vector<std::shared_ptr<output_buffer_c>> output_buffers;
    void output_pipe_c::drain_message(std::string &message);
public:
    output_pipe_c();
    virtual ~output_pipe_c();
    virtual void add_output_buffer(std::shared_ptr<output_buffer_c> output_buffer_param);
    virtual void bufferize();
    virtual pipe_t get_pipe();
    size_t get_buffer_count() const {
        return output_buffers.size();
    }
    void write_buffer(int buffer_index, const void *data, size_t size) {
        last_buffer_ = output_buffers[buffer_index];
        output_buffers[buffer_index]->write(data, size);
    }
    const std::shared_ptr<output_buffer_c> get_output_buffer(int i) const {
        return output_buffers[i];
    }
    std::function<void(std::string&, output_pipe_c*)> process_message;
};
