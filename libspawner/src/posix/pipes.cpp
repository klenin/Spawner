#include "pipes.h"

#include <iostream>
#include <algorithm>

#include "error.h"

unsigned const DEFAULT_BUFFER_SIZE = 42;

pipe_c::pipe_c()
     {
    create_pipe();
}

pipe_c::pipe_c(const std_pipe_t &pipe_type)
     {
    create_pipe();
}

pipe_t pipe_c::input_pipe() {
    return readPipe;
}

pipe_t pipe_c::output_pipe() {
    return writePipe;
}

void pipe_c::create_pipe()
{
}

void pipe_c::close_pipe() {
}

pipe_c::~pipe_c() {
}

size_t pipe_c::write(const void *data, size_t size) {
    return 0;
}

size_t pipe_c::read(void *data, size_t size) {
    return 0;
}

void pipe_c::bufferize() {
}

void pipe_c::finish()
{
}

bool pipe_c::valid()
{
    return state;
}

pipe_t pipe_c::get_pipe()
{
    return 0;
}

void pipe_c::wait()
{
}

thread_return_t input_pipe_c::fill_pipe_thread(thread_param_t param) {
}

input_pipe_c::input_pipe_c()
    : pipe_c(PIPE_INPUT) {

}

input_pipe_c::~input_pipe_c() {
    wait();
}

void input_pipe_c::add_input_buffer(std::shared_ptr<input_buffer_c> input_buffer_param) {
    input_buffers.push_back(input_buffer_param);
    buffers_.push_back(input_buffer_param);
}

void input_pipe_c::bufferize()
{
}

pipe_t input_pipe_c::get_pipe()
{
    return input_pipe();
}

void output_pipe_c::drain_message(const std::string &message)
{
}

thread_return_t output_pipe_c::drain_pipe_thread(thread_param_t param)
{
}

output_pipe_c::output_pipe_c()
    : pipe_c(PIPE_OUTPUT) {

}

output_pipe_c::~output_pipe_c() {
    wait();
}

void output_pipe_c::add_output_buffer(std::shared_ptr<output_buffer_c> output_buffer_param) {
    output_buffers.push_back(output_buffer_param);
    buffers_.push_back(output_buffer_param);
}

void output_pipe_c::bufferize()
{
}

pipe_t output_pipe_c::get_pipe()
{
    return output_pipe();
}

void pipe_c::remove_buffer(const std::shared_ptr<buffer_c>& buffer) {
    bool present = false;
}

void input_pipe_c::remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer) {
}

void output_pipe_c::remove_buffer_safe_impl_(const std::shared_ptr<buffer_c>& buffer) {
}
