#include "spawner_base.h"

pipe_broadcaster_ptr spawner_base_c::get_or_create_file_pipe(const std::string& path, pipe_mode mode) {
    auto file_pipe = file_pipes.find(path);
    if (file_pipe == file_pipes.end()) {
        auto file = mode == read_mode ? pipe_broadcaster::open_file(path) : pipe_broadcaster::create_file(path);
        file_pipes[path] = file;
        return file;
    }
    return file_pipe->second;
}

pipe_broadcaster_ptr spawner_base_c::get_std(std_stream_type type) {
    switch (type) {
    case std_stream_input:
        if (spawner_stdin == nullptr) {
            spawner_stdin = pipe_broadcaster::open_std(type);
        }
        return spawner_stdin;
    case std_stream_output:
        if (spawner_stdout == nullptr) {
            spawner_stdout = pipe_broadcaster::open_std(type);
        }
        return spawner_stdout;
    case std_stream_error:
        if (spawner_stderr == nullptr) {
            spawner_stderr = pipe_broadcaster::open_std(type);
        }
        return spawner_stderr;
    default:
        PANIC("Unknown std stream type");
    }

    return nullptr;
}

spawner_base_c::spawner_base_c()
    : spawner_stdin(nullptr)
    , spawner_stdout(nullptr)
    , spawner_stderr(nullptr) {

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
