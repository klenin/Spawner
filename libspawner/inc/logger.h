#ifndef LOGGER_H
#define LOGGER_H

//#define DEBUG

#ifdef DEBUG
#define LOG(...) logger::log(logger::get_log_header(__FILE__, __LINE__), __VA_ARGS__)
#else
#define LOG(...)
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

class logger {
    static std::stringstream log_data;

public:
    static long long start_time;
    static long long get_time();
    static std::string get_log_header(const char* filename, int line);

    static void log();

    template <typename First, typename... Rest>
    static void log(First&& first, Rest&&... rest);

    static void print();
};

template<typename First, typename ...Rest>
void logger::log(First && first, Rest && ...rest) {
    log_data << std::forward<First>(first) << " ";
    log(std::forward<Rest>(rest)...);
}

#endif // LOGGER_H
