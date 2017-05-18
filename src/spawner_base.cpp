#include "spawner_base.h"

multipipe_ptr spawner_base_c::get_or_create_file_pipe(const std::string& path, pipe_mode mode, options_class::redirect_flags flags) {
    auto file_pipe = file_pipes.find(path);
    if (file_pipe == file_pipes.end()) {
        auto file = mode == read_mode
            ? multipipe::open_file(path, flags.exclusive)
            : multipipe::create_file(path, flags.flush, flags.exclusive);
        file_pipes[path] = file;
        return file;
    }
    return file_pipe->second;
}

multipipe_ptr spawner_base_c::get_std(std_stream_type type, options_class::redirect_flags flags) {
    switch (type) {
    case std_stream_input:
        if (spawner_stdin == nullptr) {
            spawner_stdin = multipipe::open_std(type, flags.flush);
        }
        return spawner_stdin;
    case std_stream_output:
        if (spawner_stdout == nullptr) {
            spawner_stdout = multipipe::open_std(type, flags.flush);
        }
        return spawner_stdout;
    case std_stream_error:
        if (spawner_stderr == nullptr) {
            spawner_stderr = multipipe::open_std(type, flags.flush);
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
