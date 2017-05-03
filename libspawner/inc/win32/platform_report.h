#ifndef _SPAWNER_PLATFORM_REPORT_H_
#define _SPAWNER_PLATFORM_REPORT_H_

#include <string>

#include "inc/report.h"
#include "exceptions.h"

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

#endif//_SPAWNER_PLATFORM_REPORT_H_
