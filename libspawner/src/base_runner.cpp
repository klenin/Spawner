#include "base_runner.h"

#include "error.h"

base_runner::~base_runner() {
    finalize();
}

multipipe_ptr base_runner::get_pipe(const std_stream_type& stream_type, options_class::redirect_flags flags) {
    auto stream = streams.find(stream_type);
    if (stream == streams.end()) {
        switch (stream_type) {
        case std_stream_input:
            streams[std_stream_input] = multipipe::create_pipe(write_mode, flags.flush);
            break;
        case std_stream_output:
        case std_stream_error:
            streams[stream_type] = multipipe::create_pipe(read_mode);
            break;
        default:
            PANIC("Unknown stream type");
        }
        stream = streams.find(stream_type);
    }

    return stream->second;
}

base_runner::base_runner(const std::string& program, const options_class& options)
    : options(options)
    , program(program)
    , finalize_thread(nullptr) {
}

void base_runner::finalize() {
    if (finalize_thread != nullptr && finalize_thread->joinable()) {
        finalize_thread->join();
        delete finalize_thread;
        finalize_thread = nullptr;
    }
}
