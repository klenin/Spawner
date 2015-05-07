/*#ifndef _SPAWNER_H_
#define _SPAWNER_H_

enum spawner_state_e {
    spawner_state_ok,
    spawner_state_show_help,
    spawner_state_error_arguments,
    spawner_state_error_execution
};

class spawner_c {
protected:
    void init_options_from_arguments(options_class &options, const argument_set_c &argument_set);
    output_buffer_class *create_output_buffer(const std::string &name, const pipes_t &pipe_type, const size_t buffer_size = 4096);
    input_buffer_class *create_input_buffer(const std::string &name, const size_t buffer_size = 4096);
    runner *create_runner(session_class &session, const argument_set_c &argument_set);
    arguments_c arguments;
    std::vector<runner*> runners;
    spawner_state_e state;
public:
    spawner_c(int argc, char *argv[]);
    ~spawner_c();
    void init();
    bool run();
    std::string json_report(runner *runner_instance);
};

#endif//_SPAWNER_H_*/
