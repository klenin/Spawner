#include <inc/compatibility.h>
#include <inc/uconvert.h>
#include <inc/error.h>
#include <sstream>
#include <stdlib.h>

std::string ExitCodeToString(const unsigned int &code) {
#if defined(_WIN32) || defined(_WIN64)
    switch (code) {
        case STATUS_ACCESS_VIOLATION:
            return "AccessViolation";
        case STATUS_ARRAY_BOUNDS_EXCEEDED:
            return "ArrayBoundsExceeded";
        case STATUS_BREAKPOINT:
            return "Breakpoint";
        case STATUS_CONTROL_C_EXIT:
            return "Control_C_Exit";
        case STATUS_DATATYPE_MISALIGNMENT:
            return "DatatypeMisalignment";
        case STATUS_FLOAT_DENORMAL_OPERAND:
            return "FloatDenormalOperand";
        case STATUS_FLOAT_INEXACT_RESULT:
            return "FloatInexactResult";
        case STATUS_FLOAT_INVALID_OPERATION:
            return "FloatInvalidOperation";
        case STATUS_FLOAT_MULTIPLE_FAULTS:
            return "FloatMultipleFaults";
        case STATUS_FLOAT_MULTIPLE_TRAPS:
            return "FloatMultipleTraps";
        case STATUS_FLOAT_OVERFLOW:
            return "FloatOverflow";
        case STATUS_FLOAT_STACK_CHECK:
            return "FloatStackCheck";
        case STATUS_FLOAT_UNDERFLOW:
            return "FloatUnderflow";
        case STATUS_GUARD_PAGE_VIOLATION:
            return "GuardPageViolation";
        case STATUS_ILLEGAL_INSTRUCTION:
            return "IllegalInstruction";
        case STATUS_IN_PAGE_ERROR:
            return "InPageError";
        case STATUS_INVALID_DISPOSITION:
            return "InvalidDisposition";
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
            return "IntegerDivideByZero";
        case STATUS_INTEGER_OVERFLOW:
            return "IntegerOverflow";
        case STATUS_NONCONTINUABLE_EXCEPTION:
            return "NoncontinuableException";
        case STATUS_PRIVILEGED_INSTRUCTION:
            return "PrivilegedInstruction";
        case STATUS_REG_NAT_CONSUMPTION:
            return "RegNatConsumption";
        case STATUS_SINGLE_STEP:
            return "SingleStep";
        case STATUS_STACK_OVERFLOW:
            return "StackOverflow";
    }
#endif

    std::ostringstream s;
    s << code;
    return s.str();
}

void SetRestriction(restrictions_class &restrictions, const restriction_kind_t &restriction_kind, const std::string &value) {
    unsigned long restriction_value = restriction_no_limit;
    switch (restriction_kind) {
        case restriction_security_limit:
            restriction_value = atoi(value.c_str()/*.length()*/)?restriction_limited:restriction_no_limit;
            break;
        case restriction_processor_time_limit:
            restriction_value = convert(value_t(unit_time_second, degree_milli), value, restriction_no_limit);
            break;
        case restriction_memory_limit:
            restriction_value = convert(value_t(unit_memory_byte), value, restriction_no_limit);
            break;
        case restriction_write_limit:
            restriction_value =  convert(value_t(unit_memory_byte), value, restriction_no_limit);
            break;
        case restriction_user_time_limit:
            restriction_value = convert(value_t(unit_time_second, degree_milli), value, restriction_no_limit);
            break;
        case restriction_load_ratio:
            restriction_value = convert(value_t(unit_no_unit, degree_m4), value, restriction_no_limit);
            break;
        case restriction_idle_time_limit:
            restriction_value = convert(value_t(unit_time_second, degree_milli), value, restriction_no_limit);
            break;
        default:
            return;
    }
    restrictions.set_restriction(restriction_kind, restriction_value);

}

#if defined(_WIN32) || defined(_WIN64)
void ReadEnvironmentVariables(options_class &options, restrictions_class &restrictions) {
    CHAR buffer[1024];
    const struct {
        char *name;
        restriction_kind_t restriction;
    } restriction_bindings[] = {
        {"SP_SECURITY_LEVEL", restriction_security_limit},
        {"SP_TIME_LIMIT", restriction_processor_time_limit},
        {"SP_MEMORY_LIMIT", restriction_memory_limit},
        {"SP_WRITE_LIMIT", restriction_write_limit},
        {"SP_DEADLINE", restriction_user_time_limit},
        {"SP_LOAD_RATIO", restriction_load_ratio},
        {"SP_IDLE_TIME_LIMIT", restriction_idle_time_limit}
    };
    const int restriction_bindings_count =
        sizeof(restriction_bindings)/(sizeof(char*) + sizeof(restriction_kind_t));

    if (GetEnvironmentVariable("SP_JSON", buffer, sizeof(buffer))) {
        options.json = atoi(buffer);
    }

    if (GetEnvironmentVariable("SP_HIDE_REPORT", buffer, sizeof(buffer))) {
        options.hide_report = atoi(buffer);
    }

    if (GetEnvironmentVariable("SP_HIDE_OUTPUT", buffer, sizeof(buffer))) {
        options.hide_output = atoi(buffer);
    }
    for (int i = 0; i < restriction_bindings_count; ++i) {
        if (GetEnvironmentVariable(restriction_bindings[i].name, buffer, sizeof(buffer))) {
            SetRestriction(restrictions, restriction_bindings[i].restriction, buffer);
        }
    }
    if (GetEnvironmentVariable("SP_USER", buffer, sizeof(buffer))) {
        options.login = buffer;
    }

    if (GetEnvironmentVariable("SP_PASSWORD", buffer, sizeof(buffer))) {
        options.password = buffer;
    }

    if (GetEnvironmentVariable("SP_REPORT_FILE", buffer, sizeof(buffer))) {
        options.report_file = buffer;
    }

    if (GetEnvironmentVariable("SP_OUTPUT_FILE", buffer, sizeof(buffer))) {
        options.stdoutput.push_back(buffer);
    }

    if (GetEnvironmentVariable("SP_ERROR_FILE", buffer, sizeof(buffer))) {
        options.stderror.push_back(buffer);
    }

    if (GetEnvironmentVariable("SP_INPUT_FILE", buffer, sizeof(buffer))) {
        options.stdinput.push_back(buffer);
    }
}
#endif

std::string GenerateSpawnerReport(const report_class &rep, const options_class &options, const restrictions_class &restrictions) {
    std::ostringstream osstream;
    osstream << std::endl << "--------------- Spawner report ---------------" << std::endl;
    osstream << "Application:               " << rep.application_name << std::endl;
    osstream << "Parameters:                " << options.format_arguments() << std::endl;
    osstream << "SecurityLevel:             " << (restrictions.get_restriction(restriction_security_limit) == restriction_limited) << std::endl;
    osstream << "CreateProcessMethod:       " << (options.login==""?"CreateProcess":"WithLogon") << std::endl;
#if defined(_WIN32) || defined(_WIN64)    
    osstream << "UserName:                  " << rep.login.c_str() << std::endl;
#else // switch back from wide
    osstream << "UserName:                  " << w2a(rep.login.c_str()) << std::endl;
#endif
    osstream << "UserTimeLimit:             " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), restrictions.get_restriction(restriction_processor_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "DeadLine:                  " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "MemoryLimit:               " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "WriteLimit:                " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "UserTime:                  " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), rep.processor_time, " (sec)") << std::endl;
    osstream << "PeakMemoryUsed:            " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (Mb)") << std::endl;
    osstream << "Written:                   " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.write_transfer_count, " (Mb)") << std::endl;
    osstream << "TerminateReason:           " << get_terminate_reason(rep.terminate_reason) << std::endl;
    osstream << "ExitStatus:                ";
#if defined(_WIN32) || defined(_WIN64)
    osstream << ExitCodeToString(rep.exit_code);
#else
    if (rep.signum)
        osstream << get_signal_info(rep.signum, "%n (%t)");
    else
	osstream << ExitCodeToString(rep.exit_code);
#endif
    osstream << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "SpawnerError:              " << get_error_text() << std::endl;
    return osstream.str();
}
