#include "base_runner.h"


base_runner::base_runner (const std::string &program, const options_class &options)
        : program(program)
        , options(options)
        , process_status(process_not_started)
{
}

void base_runner::set_pipe(const pipes_t &pipe_type, std::shared_ptr<pipe_c> pipe_object) {
    pipes[pipe_type] = pipe_object;
}

std::shared_ptr<pipe_c> base_runner::get_pipe(const pipes_t &pipe_type) {
    if (pipes.find(pipe_type) == pipes.end()) {
        return 0;
    }
    return pipes[pipe_type];
}

std::shared_ptr<input_pipe_c> base_runner::get_input_pipe() {
    return std::static_pointer_cast<input_pipe_c>(get_pipe(STD_INPUT_PIPE));
}

std::shared_ptr<output_pipe_c> base_runner::get_output_pipe() {
    return std::static_pointer_cast<output_pipe_c>(get_pipe(STD_INPUT_PIPE));
}
