#ifndef _SPAWNER_PIPES_H_
#define _SPAWNER_PIPES_H_

#include <inc/platform.h>
#include <inc/buffer.h>
#include <inc/session.h>
#include <sstream>
#include <fstream>
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
    bool create_named_pipe();
    handle_t buffer_thread;
    handle_t reading_mutex;
    pipe_t readPipe, writePipe;
    std_pipe_t pipe_type;
    std::string name;
    bool state;
    pipe_t input_pipe();
    pipe_t output_pipe();
public:
    pipe_class();
    pipe_class(const std_pipe_t &pipe_type);
	pipe_class(const session_class &session, const std::string &pipe_name, const std_pipe_t &pipe_type, const bool &create = true);
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
    input_buffer_class *input_buffer;
    static thread_return_t writing_buffer(thread_param_t param);
public:
    input_pipe_class();
    input_pipe_class(input_buffer_class *input_buffer_param);
    input_pipe_class(const session_class &session, const std::string &pipe_name, input_buffer_class *input_buffer_param, const bool &create = true);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

class output_pipe_class: public pipe_class
{
protected:
    output_buffer_class *output_buffer;
    static thread_return_t reading_buffer(thread_param_t param);
public:
    output_pipe_class();
    output_pipe_class(output_buffer_class *output_buffer_param);
    output_pipe_class(const session_class &session, const std::string &pipe_name, output_buffer_class *output_buffer_param, const bool &create = true);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

#endif//_SPAWNER_PIPES_H_
