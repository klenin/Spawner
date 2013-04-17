#ifndef _SPAWNER_BUFFER_H_
#define _SPAWNER_BUFFER_H_

#include <inc/platform.h>
#include <string>
#include <sstream>
#include <queue>
#include <time.h>

const unsigned int BUFFER_SIZE = 4096;//provide this in to constructor


class input_buffer_class {
protected:
    size_t buffer_size;
public:
    input_buffer_class();
    input_buffer_class(const size_t &buffer_size_param);
    virtual bool readable() {
        return false;
    }
    virtual size_t read(void *data, size_t size) {
        return 0;
    }
    virtual size_t get_buffer_size() {
        return buffer_size;
    }
};
class output_buffer_class {
protected:
    size_t buffer_size;
public:
    output_buffer_class();
    output_buffer_class(const size_t &buffer_size_param);
    virtual bool writeable() {
        return false;
    }
    virtual size_t write(void *data, size_t size) {
        return 0;
    }
    virtual size_t get_buffer_size() {
        return buffer_size;
    }
};

class output_stream_buffer_class: public output_buffer_class {
private:
    std::queue<std::string> incoming, shadow_incoming;
    handle_t write_mutex;
    double last_write_time;
public:
    output_stream_buffer_class(const size_t &buffer_size_param): output_buffer_class(buffer_size_param) {
        write_mutex = CreateMutex(NULL, FALSE, NULL);
        if (!write_mutex) {
            //error
        }
    }
    virtual bool writeable() {
        return true;
    }
    bool ready() {
        return incoming.size() != 0;
    }
    std::string stock() {
        std::string result;
        while (incoming.size()) {
            result += incoming.front();
            incoming.pop();
        }
        return result;
    }
    virtual size_t write(void *data, size_t size) {
        WaitForSingleObject(write_mutex, INFINITE);

        std::string s((char*)data,size);
        incoming.push(s);
        last_write_time = (double)clock()/CLOCKS_PER_SEC;
        ReleaseMutex(write_mutex);
        return size;
    }

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
            Sleep(10);
        std::string s = buffer.str();
        size = min(size, (size_t)s.length());
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
public:
    output_stdout_buffer_class();
    output_stdout_buffer_class(const size_t &buffer_size_param);
    virtual bool writeable();
    virtual size_t write(void *data, size_t size);
};


#endif//_SPAWNER_BUFFER_H_
