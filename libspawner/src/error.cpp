#include "error.h"
#include "platform.h"

#include <cstring>
#include <sstream>

void abort_at_panic_();
void begin_panic_();
bool do_we_panic_();
void exec_on_panic_action_();

void panic_(const std::string& error_message, const char* filename, int line_number) {
    std::stringstream error_text;
    const char *fn = filename + strlen(filename);
    while (fn > filename && fn[-1] != '\\' && fn[-1] != '/') --fn;
    error_text << fn << ":" << line_number << ": " << error_message;
    set_error_text(error_text.str());
    if (!do_we_panic_()) {
        begin_panic_();
        exec_on_panic_action_();
        abort_at_panic_();
    }
}

void abort_at_panic_()
{
#if defined(_DEBUG)
    throw std::runtime_error("");
#else
    platform_exit_failure();
    //::abort();
#endif
}

static std::function<void()> on_panic_action_ = nullptr;
static std::string error_text_ = "<none>";
static bool we_do_panic_ = false;

void begin_panic_() {
    // TODO: use atomic
    we_do_panic_ = true;
}

bool do_we_panic_() {
    return we_do_panic_;
}

void set_error_text(const std::string& error_text) {
    if (do_we_panic_()) {
        error_text_ = error_text_ + ", " + error_text;
    }
    else {
        error_text_ = error_text;
    }
}

const std::string& get_error_text() {
    return error_text_;
}

void set_on_panic_action(const std::function<void()>& action) {
    on_panic_action_ = action;
}

void exec_on_panic_action_() {
    if (on_panic_action_) {
        on_panic_action_();
    }
}
