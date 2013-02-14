#ifndef _SPAWNER_PIPES_H_
#define _SPAWNER_PIPES_H_

#include "platform.h"
#include <sstream>
#include <ostream>
#include <istream>
#include <string>

enum pipes_t//rename
{
    STD_INPUT_PIPE,
    STD_OUTPUT_PIPE,
    STD_ERROR_PIPE
};

enum std_pipe_t
{
    PIPE_INPUT,
    PIPE_OUTPUT,
};

class pipe_class
{
protected:
    bool create_pipe();
    handle_t buffer_thread;
    handle_t reading_mutex;
    std::istringstream pipe_buffer;
    size_t buff_size;
    pipe_t readPipe, writePipe;
    std_pipe_t pipe_type;
	std::string file_name;
    bool state;
public:
    pipe_class(std_pipe_t pipe_type);
	pipe_class(std::string file_name, std_pipe_t pipe_type);
    pipe_t read_pipe(){ return readPipe;}
    pipe_t write_pipe(){ return writePipe;}
    void close_pipe();
    ~pipe_class();
    size_t write(void *data, size_t size);
    size_t read(void *data, size_t size);
    virtual bool bufferize();
    void wait();
    void finish();
    /* think about safer way of reading from pipe */
    std::istringstream &stream();
    size_t buffer_size();
    void wait_for_pipe(const unsigned int &ms_time);
    void safe_release();
    bool valid();
    virtual pipe_t get_pipe();
};

class abstract_pipe_class {
protected:
	handle_t handle;
public:
	abstract handle_t read_pipe();
	abstract handle_t write_pipe();
};

class handle_pipe_class: public pipe_class {
public:
	handle_pipe_class(handle_t handle): handle(handle) {
	}
	~handle_pipe_class() {
	}
};

class input_pipe_class: public pipe_class {
	input_pipe_class() {}
	read_pipe() {
	}
};

class read_pipe_class: public pipe_class
{
protected:
    std::istream &input_stream;
    static thread_return_t writing_buffer(thread_param_t param);
public:
    read_pipe_class();
    read_pipe_class(std::istream &input_stream);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

class write_pipe_class: public pipe_class
{
protected:
    std::ostream &output_stream;
    static thread_return_t reading_buffer(thread_param_t param);
public:
    write_pipe_class();
    write_pipe_class(std::ostream &output_stream);
    virtual bool bufferize();
    virtual pipe_t get_pipe();
};

#endif//_SPAWNER_PIPES_H_
