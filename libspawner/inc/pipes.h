#ifndef _SPAWNER_PIPES_H_
#define _SPAWNER_PIPES_H_

#include <inc/platform.h>
#include <inc/buffer.h>
#include <inc/session.h>
#include <sstream>
#include <string>

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
    ~pipe_class();
    size_t write(void *data, size_t size);
    size_t read(void *data, size_t size);
    virtual bool bufferize();
    void wait();
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
    std::vector<input_buffer_class *> input_buffers;
    static thread_return_t writing_buffer(thread_param_t param);
public:
    input_pipe_class();
    input_pipe_class(input_buffer_class *input_buffer_param);
    input_pipe_class(std::vector<input_buffer_class *> input_buffer_param);
    virtual void add_input_buffer(input_buffer_class *input_buffer_param);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

class output_pipe_class: public pipe_class
{
protected:
    std::vector<output_buffer_class *> output_buffers;
    static thread_return_t reading_buffer(thread_param_t param);
public:
    output_pipe_class();
    output_pipe_class(output_buffer_class *output_buffer_param);
    output_pipe_class(std::vector<output_buffer_class *> output_buffer_param);
    virtual void add_output_buffer(output_buffer_class *output_buffer_param);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

#endif//_SPAWNER_PIPES_H_
