#ifndef _SPAWNER_PIPES_H_
#define _SPAWNER_PIPES_H_

#include "platform.h"
#include <sstream>
#include <string>
#include <iostream>

typedef enum
{
    STD_INPUT   = 0x02,
    STD_OUTPUT  = 0x01,
    STD_ERROR   = 0x05,
}std_pipe_t;

// #TODO rename
const unsigned int STD_PIPE_IO = 0x1;// input->output Read pipe
const unsigned int STD_PIPE_OI = 0x2;// output<-input Write pipe

const unsigned int BUFFER_SIZE = 4096;


class pipe_class
{
public:
    pipe_class();
    pipe_class(std_pipe_t handleType);
    void init();
    pipe_t read_pipe(){ return readPipe;}
    pipe_t write_pipe(){ return writePipe;}
    void close_pipe();
    ~pipe_class();
    bool write(void *data, size_t size);
    size_t read(void *data, size_t size);
    void bufferize();
    void wait();
    void finish();
    /* think about safer way of reading from pipe */
    std::istringstream &stream();
    size_t buffer_size();
    void wait_for_pipe(const unsigned int &ms_time);
private:
    handle_t reading_thread;
    handle_t reading_mutex;
    std::istringstream reading_buffer;
    size_t buff_size;
    static thread_return_t reading_body(thread_param_t param);
    pipe_t readPipe, writePipe;
    std_pipe_t pipe_type;
};

#endif//_SPAWNER_PIPES_H_
