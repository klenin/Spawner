#ifndef _SPAWNER_PIPES_H_
#define _SPAWNER_PIPES_H_

#include "platform.h"
#include <sstream>
#include <string>

enum std_pipe_t
{
    STD_INPUT   = 0x02,
    STD_OUTPUT  = 0x01,
    STD_ERROR   = 0x05,
};

// #TODO rename


class pipe_class
{
public:
    pipe_class(std_pipe_t handleType);
    pipe_t read_pipe(){ return readPipe;}
    pipe_t write_pipe(){ return writePipe;}
    void close_pipe();
    ~pipe_class();
    size_t write(void *data, size_t size);
    size_t read(void *data, size_t size);
    bool bufferize();
    void wait();
    void finish();
    /* think about safer way of reading from pipe */
    void set_stream(const std::string &name);
    std::istringstream &stream();
    size_t buffer_size();
    void wait_for_pipe(const unsigned int &ms_time);
    void safe_release();
    bool valid();
private:
    pipe_class(){}
    std::string stream_name;
    handle_t buffer_thread;
    handle_t reading_mutex;
    std::istringstream pipe_buffer;
    size_t buff_size;
    static thread_return_t writing_buffer(thread_param_t param);
    static thread_return_t reading_buffer(thread_param_t param);
    pipe_t readPipe, writePipe;
    std_pipe_t pipe_type;
    bool state;
};

#endif//_SPAWNER_PIPES_H_
