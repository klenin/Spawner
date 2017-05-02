#ifndef _SPAWNER_PLATFORM_REPORT_H_
#define _SPAWNER_PLATFORM_REPORT_H_

#include <string>

#include "signals.h"

class report_class: public base_report_class
{
public:
    report_class():
        signum(signal_signal_no) {}
    signal_t signum;
};

typedef signal_t event_t;

const char *get_event_name(event_t event);
const char *get_event_text(event_t event);
std::string get_event_info(event_t event, std::string format);

#endif //_SPAWNER_PLATFORM_REPORT_H_
