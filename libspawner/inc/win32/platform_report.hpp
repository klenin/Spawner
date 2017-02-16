#ifndef _SP_PLATFORM_REPORT_HPP_
#define _SP_PLATFORM_REPORT_HPP_

#include <string>

#include "inc/report.hpp"
#include "exceptions.hpp"

class report_class: public base_report_class
{
public:
    report_class():
        exception(exception_exception_no) {}
    exception_t exception;
};

typedef exception_t event_t;

const char *get_event_name(event_t event);
const char *get_event_text(event_t event);
std::string get_event_info(event_t event, std::string format);

#endif // _SP_PLATFORM_REPORT_HPP_
