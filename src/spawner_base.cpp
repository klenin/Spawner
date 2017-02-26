#include "spawner_base.hpp"

static std::map<std::string, std::shared_ptr<output_buffer_c>> output_cache;

std::shared_ptr<output_buffer_c> spawner_base_c::create_output_buffer(const std::string &name,
    const pipes_t &pipe_type) {
    std::shared_ptr<output_buffer_c> output_buffer = nullptr;
    if (name == "std") {
        unsigned int color = FOREGROUND_BLUE | FOREGROUND_GREEN;
        if (pipe_type == STD_ERROR_PIPE) {
            color = FOREGROUND_RED | FOREGROUND_INTENSITY;
        }
        output_buffer = std::make_shared<output_stdout_buffer_c>(color);
    }
    else if (name[0] == '*') {
    }
    else if (name.length()) {
        std::map<std::string, std::shared_ptr<output_buffer_c>>::iterator it = output_cache.find(name);
        if (it != output_cache.end()) {
            output_buffer = it->second;
        }
        else {
            output_buffer = std::make_shared<output_file_buffer_c>(name);
            output_cache[name] = output_buffer;
        }
    }
    return output_buffer;
}

std::shared_ptr<input_buffer_c> spawner_base_c::create_input_buffer(const std::string &name) {
    std::shared_ptr<input_buffer_c> input_buffer = nullptr;
    if (name == "std") {
        input_buffer = std::make_shared<input_stdin_buffer_c>();
    }
    else if (name[0] == '*') {
    }
    else if (name.length()) {
        input_buffer = std::make_shared<input_file_buffer_c>(name);
    }
    return input_buffer;
}

spawner_base_c::spawner_base_c() {

}

spawner_base_c::~spawner_base_c() {

}

void spawner_base_c::begin_report() {

}

void spawner_base_c::run(int argc, char *argv[]) {

}

void spawner_base_c::run() {

}

std::string spawner_base_c::help() {
    return "Usage:\n\t--legacy=<sp99|sp00|pcms2>\n";
}

void spawner_base_c::init_arguments() {

}

bool spawner_base_c::init() {
    return true;
}

void spawner_base_c::print_report() {

}
