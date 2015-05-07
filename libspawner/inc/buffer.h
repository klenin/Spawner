#ifndef _SPAWNER_BUFFER_H_
#define _SPAWNER_BUFFER_H_

#include <inc/platform.h>
#include <string>
#include <sstream>
#include <queue>
#include <time.h>
#include <math.h>
const unsigned int BUFFER_SIZE = 4096;//provide this in to constructor

//dirty hack
//#ifndef min
#define min_def(a, b) (a) > (b) ? (b) : (a)
//#endif

class buffer_class {
protected:
    size_t buffer_size;
public:
    buffer_class();
    buffer_class(const size_t &buffer_size_param);
    virtual size_t get_buffer_size() {
        return buffer_size;
    }
};

class input_buffer_class: public virtual buffer_class {
public:
    input_buffer_class();
    input_buffer_class(const size_t &buffer_size_param);
    virtual bool readable() {
        return false;
    }
    virtual size_t read(void *data, size_t size) {
        return 0;
    }
};
class output_buffer_class: public virtual buffer_class {
public:
    output_buffer_class();
    output_buffer_class(const size_t &buffer_size_param);
    virtual bool writeable() {
        return false;
    }
    virtual size_t write(void *data, size_t size) {
        return 0;
    }
};
class duplex_buffer_class: public input_buffer_class, public output_buffer_class {
protected:
    handle_t in;
    handle_t out;
public:
    duplex_buffer_class();
    virtual bool readable();
    virtual bool writeable();
    virtual size_t read(void *data, size_t size);
    virtual size_t write(void *data, size_t size);
};

class input_stream_buffer_class: public input_buffer_class {
protected:
    bool ready;
    size_t offset;
    //CONDITION_VARIABLE condition_variable;
public:
    std::ostringstream buffer;
    input_stream_buffer_class(): ready(false), input_buffer_class(0), offset(0) {
        //InitializeConditionVariable(&condition_variable);
    }
    virtual bool readable() {
        return true;
    }
    virtual size_t get_buffer_size() {
        while (!ready);
        return (size_t)buffer.tellp();
    }
    void set_ready() {
        offset = 0;
        ready = true;
    }

    virtual size_t read(void *data, size_t size) {
        while (!ready)
            Sleep(5);
        std::string s = buffer.str();
        size = min_def(size, (size_t)s.length());
        memcpy(data, s.c_str(), size);
        buffer.str(s.substr(offset));
        ready = false;

        return size;
    }
};

class handle_buffer_class {
protected:
    handle_t stream;
    size_t protected_read(void *data, size_t size);
    size_t protected_write(void *data, size_t size);
    void init_handle(handle_t stream_arg);
public:
    handle_buffer_class();
    ~handle_buffer_class();
};

class input_file_buffer_class: public input_buffer_class, protected handle_buffer_class {
public:
    input_file_buffer_class();
    input_file_buffer_class(const std::string &file_name, const size_t &buffer_size_param);
    virtual bool readable();
    virtual size_t read(void *data, size_t size);
};

class output_file_buffer_class: public output_buffer_class, protected handle_buffer_class {
public:
    output_file_buffer_class();
    output_file_buffer_class(const std::string &file_name, const size_t &buffer_size_param);
    virtual bool writeable();
    virtual size_t write(void *data, size_t size);
};

class output_stdout_buffer_class: public output_buffer_class, protected handle_buffer_class {
protected:
    unsigned int color;
public:
    output_stdout_buffer_class();
    output_stdout_buffer_class(const size_t &buffer_size_param = BUFFER_SIZE, const unsigned int &color_param = 0);
    virtual bool writeable();
    virtual size_t write(void *data, size_t size);
};

class input_stdin_buffer_class: public input_buffer_class, protected handle_buffer_class {
protected:
    unsigned int color;
public:
    input_stdin_buffer_class();
    input_stdin_buffer_class(const size_t &buffer_size_param = BUFFER_SIZE);
    virtual bool readable();
    virtual size_t read(void *data, size_t size);
};

#endif//_SPAWNER_BUFFER_H_
