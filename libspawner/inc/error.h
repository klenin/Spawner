#pragma once

#include <string>
#include <functional>
#include <Windows.h>

std::string get_stacktrace_string();
void make_minidump(EXCEPTION_POINTERS* e);
void set_on_panic_action(const std::function<void()> action);
void set_error_text(const std::string& error_text);
const std::string& get_error_text();

void panic_(const std::string& error_message, char* filename, int line_number);

#define PANIC(MESSAGE) do { panic_(MESSAGE, __FILE__, __LINE__); } while (false);

#define PANIC_IF(CONDITION) do { \
    if (CONDITION) { \
        PANIC(#CONDITION); \
    } \
} while (false); \

class finally final {
    typedef std::function<void()> handler_t_;
public:
    finally(handler_t_ handler)
        : handler_(handler) {
    }
    finally() = delete;
    finally(const finally&) = delete;
    ~finally() {
        handler_();
    }
private:
    handler_t_ handler_;
};
