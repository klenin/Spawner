#pragma once

#include <sstream>
#include <string>
#include <memory>

#include "inc/platform.h"
#include "inc/buffer.h"
#include "inc/session.h"

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

class pipe_class
{
protected:
    bool create_pipe();
    handle_t buffer_thread;
    handle_t reading_mutex;
    pipe_t readPipe, writePipe;
    std_pipe_t pipe_type;
    bool state;
    pipe_t input_pipe();
    pipe_t output_pipe();
public:
    pipe_class();
    pipe_class(const std_pipe_t &pipe_type);
    void close_pipe();
    virtual ~pipe_class();
    size_t write(const void *data, size_t size);
    size_t read(void *data, size_t size);
    virtual bool bufferize();
//    void wait();
    void finish();
    /* think about safer way of reading from pipe */
    void wait_for_pipe(const unsigned int &ms_time);
    void safe_release();
    bool valid();
    virtual pipe_t get_pipe();
};

class input_pipe_class: public pipe_class
{
protected:
    std::vector<std::shared_ptr<input_buffer_c>> input_buffers;
    static thread_return_t writing_buffer(thread_param_t param);
public:
    input_pipe_class();
    virtual ~input_pipe_class();
    input_pipe_class(std::shared_ptr<input_buffer_c> input_buffer_param);
    input_pipe_class(std::vector<std::shared_ptr<input_buffer_c>> input_buffer_param);
    virtual void add_input_buffer(std::shared_ptr<input_buffer_c> input_buffer_param);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

class output_pipe_class: public pipe_class
{
protected:
    std::vector<std::shared_ptr<output_buffer_c>> output_buffers;
    static thread_return_t reading_buffer(thread_param_t param);
public:
    output_pipe_class();
    virtual ~output_pipe_class();
    output_pipe_class(std::shared_ptr<output_buffer_c> output_buffer_param);
    output_pipe_class(std::vector<std::shared_ptr<output_buffer_c>> output_buffer_param);
    virtual void add_output_buffer(std::shared_ptr<output_buffer_c> output_buffer_param);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};
