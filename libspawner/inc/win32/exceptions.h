#ifndef _SPAWNER_EXCEPTIONS_H_
#define _SPAWNER_EXCEPTIONS_H_

#include <Windows.h>
#include "inc/report.h"

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

// Note all 'thread' replaced with 'process'
const map_cell event_identifiers[] = {
    {exception_access_violation         , "EXCEPTION_ACCESS_VIOLATION"
        ,"The Process tried to read from or write to a virtual address for which it does not have the appropriate access."},
    {exception_datatype_misalign        , "EXCEPTION_DATATYPE_MISALIGNMENT"
        ,"The Process tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on."},
    {exception_breakpoint               , "EXCEPTION_BREAKPOINT"
        ,"A breakpoint was encountered."},
    {exception_single_step              , "EXCEPTION_SINGLE_STEP"
        ,"A trace trap or other single-instruction mechanism signaled that one instruction has been executed."},
    {exception_array_bounds_exceeded    , "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"
        ,"The Process tried to access an array element that is out of bounds and the underlying hardware supports bounds checking."},
    {exception_flt_denormal_operand     , "EXCEPTION_FLT_DENORMAL_OPERAND"
        ,"One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value."},
    {exception_flt_divide_by_zero       , "EXCEPTION_FLT_DIVIDE_BY_ZERO"
        ,"The Process tried to divide a floating-point value by a floating-point divisor of zero."},
    {exception_flt_inexact_result       , "EXCEPTION_FLT_INEXACT_RESULT"
        ,"The result of a floating-point operation cannot be represented exactly as a decimal fraction."},
    {exception_flt_invalid_operation    , "EXCEPTION_FLT_INVALID_OPERATION"
        ,"This exception represents any floating-point exception not included in this list."},
    {exception_flt_overflow             , "EXCEPTION_FLT_OVERFLOW"
        ,"The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type."},
    {exception_flt_stack_check          , "EXCEPTION_FLT_STACK_CHECK"
        ,"The stack overflowed or underflowed as the result of a floating-point operation."},
    {exception_flt_underflow            , "EXCEPTION_FLT_UNDERFLOW"
        ,"The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type."},
    {exception_int_divide_by_zero       , "EXCEPTION_INT_DIVIDE_BY_ZERO"
        ,"The Process tried to divide an integer value by an integer divisor of zero."},
    {exception_int_overflow             , "EXCEPTION_INT_OVERFLOW"
        ,"The result of an integer operation caused a carry out of the most significant bit of the result."},
    {exception_priv_instruction         , "EXCEPTION_PRIV_INSTRUCTION"
        ,"The Process tried to execute an instruction whose operation is not allowed in the current machine mode."},
    {exception_in_page_error            , "EXCEPTION_IN_PAGE_ERROR"
        ,"The Process tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network."},
    {exception_illegal_instruction      , "EXCEPTION_ILLEGAL_INSTRUCTION"
        ,"The Process tried to execute an invalid instruction."},
    {exception_noncontinuable_exception , "EXCEPTION_NONCONTINUABLE_EXCEPTION"
        ,"The Process tried to continue execution after a noncontinuable exception occurred."},
    {exception_stack_overflow           , "EXCEPTION_STACK_OVERFLOW"
        ,"The Process used up its stack."},
    {exception_invalid_disposition      , "EXCEPTION_INVALID_DISPOSITION"
        ,"An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception."},
    {exception_guard_page               , "EXCEPTION_GUARD_PAGE"
        ,"A page of memory that marks the end of a data structure, such as a stack or an array, has been accessed."},
    {exception_invalid_handle           , "EXCEPTION_INVALID_HANDLE"
        ,"Invalid handle."}, // fix this
    {exception_exception_no             , "EXCEPTION_NO", ""},
    {exception_unknown                  , nullptr, "EXCEPTION_UNKNOWN"},
};

#endif//_SPAWNER_EXCEPTIONS_H_
