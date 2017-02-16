#ifndef _SP_STATUS_HPP_
#define _SP_STATUS_HPP_

enum process_status_t
{
    process_still_active        = 0x2, //b 0000 0010
    process_suspended           = 0x6, //b 0000 0110 // stopped by a signal
    process_finished_normal     = 0x1, //b 0000 0001 // exit return from main
    process_finished_abnormally = 0x5, //b 0000 0101 // coredump
    process_finished_terminated = 0x9, //b 0000 1001 // terminated by a signal
    process_not_started         = 0x80,//b 1000 0000
    process_spawner_crash    = 0x90,   //b 1001 0000
};

enum terminate_reason_t
{
    terminate_reason_not_terminated = 0x0,
    terminate_reason_none,
    terminate_reason_abnormal_exit_process,
    terminate_reason_time_limit,
    terminate_reason_write_limit,
    terminate_reason_memory_limit,
    terminate_reason_user_time_limit,
    terminate_reason_load_ratio_limit,
    terminate_reason_debug_event,
    terminate_reason_created_process,
    terminate_reason_by_controller,
};

#endif // _SP_STATUS_HPP_
