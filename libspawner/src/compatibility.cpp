#include "inc/compatibility.hpp"

#include "inc/uconvert.hpp"
#include "inc/error.hpp"
#include <sstream>

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

std::string GenerateSpawnerReport(const report_class &rep, const options_class &options, const restrictions_class &restrictions) {
    std::ostringstream osstream;
    osstream << std::endl << "--------------- Spawner report ---------------" << std::endl;
    osstream << "Application:               " << rep.application_name << std::endl;
    osstream << "Parameters:                " << options.format_arguments() << std::endl;
    osstream << "SecurityLevel:             " << (restrictions.get_restriction(restriction_security_limit) == restriction_limited) << std::endl;
    osstream << "CreateProcessMethod:       " << (options.login==""?"CreateProcess":"WithLogon") << std::endl;
    osstream << "UserName:                  " << w2a(rep.login.c_str()) << std::endl;
    osstream << "UserTimeLimit:             " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), restrictions.get_restriction(restriction_processor_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "DeadLine:                  " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), restrictions.get_restriction(restriction_user_time_limit), " (u)", restriction_no_limit) << std::endl;
    osstream << "MemoryLimit:               " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_memory_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "WriteLimit:                " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), restrictions.get_restriction(restriction_write_limit), " (du)", restriction_no_limit) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "UserTime:                  " << convert(value_t(unit_time_second, degree_micro), value_t(unit_time_second), (long double)rep.processor_time, " (sec)") << std::endl;
    osstream << "PeakMemoryUsed:            " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), rep.peak_memory_used, " (Mb)") << std::endl;
    osstream << "Written:                   " << convert(value_t(unit_memory_byte), value_t(unit_memory_byte, degree_mega), (long double)rep.write_transfer_count, " (Mb)") << std::endl;
    osstream << "TerminateReason:           " << get_terminate_reason(rep.terminate_reason) << std::endl;
    osstream << "ExitStatus:                " << ExtractExitStatus(rep) << std::endl;
    osstream << "----------------------------------------------" << std::endl;
    osstream << "SpawnerError:              " << get_error_text() << std::endl;
    return osstream.str();
}
