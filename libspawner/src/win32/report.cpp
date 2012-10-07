#include "report.h"

typedef struct 
{
    unsigned long key;
    char *name;
    char *text;
} map_cell;

// Note all 'thread' replaced with 'process'
const map_cell exception_identifiers[] = {
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
};

const unsigned int process_status_descriptions_count = 6;
const unsigned int terminate_reason_descriptions_count = 5;

typedef struct
{
    process_status_t process_status;
    char *name;
} process_status_description;

const process_status_description process_status_descriptions[] = {
    {process_still_active,        "PROCESS_STILL_ACTIVE"},
    {process_suspended,           "PROCESS_SUSPENDED"},
    {process_finished_normal,     "PROCESS_FINISHED_NORMAL"},
    {process_finished_abnormally, "PROCESS_FINISHED_ABNORMALLY"},
    {process_finished_terminated, "PROCESS_FINISHED_TERMINATED"},
    {process_not_started,         "PROCESS_NOT_STARTED"},
};

typedef struct
{
    terminate_reason_t terminate_reason;
    char *name;
} terminate_reason_description;

const terminate_reason_description terminate_reason_descriptions[] = {
    {terminate_reason_not_terminated,  "TERMINATE_REASON_NOT_TERMINATED"},
    {terminate_reason_time_limit,      "TERMINATE_REASON_TIME_LIMIT"},
    {terminate_reason_write_limit,     "TERMINATE_REASON_WRITE_LIMIT"},
    {terminate_reason_memory_limit,    "TERMINATE_REASON_MEMORY_LIMIT"},
    {terminate_reason_user_time_limit, "TERMINATE_REASON_USER_TIME_LIMIT"},
};

unsigned int get_exception_index(exception_t exception)
{
    //TODO implement with map
    for (unsigned int k = 0; k < sizeof(exception_identifiers)/sizeof(map_cell); k++)
        if (exception_identifiers[k].key == exception)
            return k;
    //TODO fix error
    return 0;
}

const char *get_exception_name(unsigned int index)
{
    return exception_identifiers[index].name;
}

const char * get_exception_name(exception_t exception)
{
    unsigned int index = get_exception_index(exception);
    return exception_identifiers[index].name;
}

const char *get_exception_text(unsigned int index)
{
    return exception_identifiers[index].text;
}

const char *get_exception_text(exception_t exception)
{
    unsigned int index = get_exception_index(exception);
    return exception_identifiers[index].text;
}

std::string get_exception_info(exception_t exception, std::string format)
{
    string res = format;
    unsigned int index = get_exception_index(exception);
    size_t pos = 0;
    while ((pos = res.find("%n")) != string::npos)
        res.replace(pos, 2, get_exception_name(index));
    while ((pos = res.find("%t")) != string::npos)
        res.replace(pos, 2, get_exception_text(index));
    //std::replace(res.begin(), res.end(), string("%n"), string(get_exception_name(index)));
    //std::replace(res.begin(), res.end(), string("%t"), string(get_exception_text(index)));
    return res;
}

std::string get_status_text(process_status_t process_status)
{
    for (unsigned int i = 0; i < process_status_descriptions_count; i++)
        if (process_status_descriptions[i].process_status == process_status)
            return process_status_descriptions[i].name;
    return process_status_descriptions[0].name;
}
std::string get_terminate_reason(terminate_reason_t terminate_reason)
{
    for (unsigned int i = 0; i < terminate_reason_descriptions_count; i++)
        if (terminate_reason_descriptions[i].terminate_reason == terminate_reason)
            return terminate_reason_descriptions[i].name;
    return terminate_reason_descriptions[0].name;
}

