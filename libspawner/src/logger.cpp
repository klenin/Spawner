#include "logger.h"

#include <chrono>
#include <cstdlib>

using namespace std::chrono;

static const int header_fixed_len = 25;
static const int time_fixed_len = 5;

std::stringstream logger::log_data;

long long logger::start_time = 0;

long long logger::get_time() {
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string logger::get_log_header(const char* filename, int line) {
    std::string header = filename;
    auto ll = header.find_last_of('\\');
    if (ll == std::string::npos) { ll = header.find_last_of('/');}
    if (ll != std::string::npos) { header = header.substr(ll + 1);}
    header += ":" + std::to_string(line) + ":";
    auto len = header_fixed_len - header.length();
    std::string fixed_end = " " + std::string(len > 0 ? len : 0, '.') + " ";
    std::string ms = std::to_string(get_time() - start_time);
    auto time_len = time_fixed_len - ms.length();
    std::string fixed_time_head = std::string(time_len > 0 ? time_len : 0, ' ');
    return fixed_time_head + ms + "  " + header + fixed_end;
}

void logger::log() {
    log_data << std::endl;
}

void logger::print() {
    std::string all_data = log_data.str();
    if (!all_data.empty()) {
        std::cerr << "  time\tfile:line\t\tdata" << std::endl;
        std::cerr << all_data;
    }
}
