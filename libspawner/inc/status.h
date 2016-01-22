#ifndef _SPAWNER_STATUS_H_
#define _SPAWNER_STATUS_H_

#include "platform.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include "signal.h"
#endif

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

#if defined(_WIN32) || defined(_WIN64)
enum exception_t
{
    exception_unknown                   = 3,
    exception_access_violation          = EXCEPTION_ACCESS_VIOLATION,
    exception_datatype_misalign         = EXCEPTION_DATATYPE_MISALIGNMENT,
    exception_breakpoint                = EXCEPTION_BREAKPOINT,
    exception_single_step               = EXCEPTION_SINGLE_STEP,
    exception_array_bounds_exceeded     = EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
    exception_flt_denormal_operand      = EXCEPTION_FLT_DENORMAL_OPERAND,
    exception_flt_divide_by_zero        = EXCEPTION_FLT_DIVIDE_BY_ZERO,
    exception_flt_inexact_result        = EXCEPTION_FLT_INEXACT_RESULT,
    exception_flt_invalid_operation     = EXCEPTION_FLT_INVALID_OPERATION,
    exception_flt_overflow              = EXCEPTION_FLT_OVERFLOW,
    exception_flt_stack_check           = EXCEPTION_FLT_STACK_CHECK,
    exception_flt_underflow             = EXCEPTION_FLT_UNDERFLOW,
    exception_int_divide_by_zero        = EXCEPTION_INT_DIVIDE_BY_ZERO,
    exception_int_overflow              = EXCEPTION_INT_OVERFLOW,
    exception_priv_instruction          = EXCEPTION_PRIV_INSTRUCTION,
    exception_in_page_error             = EXCEPTION_IN_PAGE_ERROR,
    exception_illegal_instruction       = EXCEPTION_ILLEGAL_INSTRUCTION,
    exception_noncontinuable_exception  = EXCEPTION_NONCONTINUABLE_EXCEPTION,
    exception_stack_overflow            = EXCEPTION_STACK_OVERFLOW,
    exception_invalid_disposition       = EXCEPTION_INVALID_DISPOSITION,
    exception_guard_page                = EXCEPTION_GUARD_PAGE,
    exception_invalid_handle            = EXCEPTION_INVALID_HANDLE,
    exception_exception_no              = 0x0,
    //exception_possible_deadlock         = EXCEPTION_POSSIBLE_DEADLOCK,
};
#else
enum signal_t
{
    signal_sighup = SIGHUP,
    signal_sigint = SIGINT,
    signal_sigquit = SIGQUIT,
    signal_sigill = SIGILL,
    signal_abort = SIGABRT,
    signal_sigfpe = SIGFPE,
    signal_sigkill = SIGKILL,
    signal_sigbus = SIGBUS,
    signal_sigsegv = SIGSEGV,
    signal_sigterm = SIGTERM,
    signal_sigstop = SIGSTOP,
    signal_sigtstp = SIGTSTP,
    signal_sigcont = SIGCONT,
    signal_sigsys = SIGSYS,
    signal_sigxcpu = SIGXCPU,
    signal_sigxfsz = SIGXFSZ,
    signal_signal_no = 0x0, // no signal recieved.
};
#endif

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

#endif//_SPAWNER_STATUS_H_
