#include "platform_report.h"

std::string get_status_text(process_status_t process_status)
{
    for (unsigned int i = 0; i < process_status_descriptions_count; i++)
        if (process_status_descriptions[i].process_status == process_status)
            return process_status_descriptions[i].name;
    return process_status_descriptions[0].name;
}

std::string get_terminate_reason(terminate_reason_t terminate_reason)
{
    int i = 0;
    while (terminate_reason_descriptions[i].name) {
        if (terminate_reason_descriptions[i].terminate_reason == terminate_reason) {
            return terminate_reason_descriptions[i].name;
        }
        i++;
    }
    return terminate_reason_descriptions[0].name;
}

unsigned int get_event_index(event_t event)
{
    unsigned int result = 0;
    const map_cell *mcs = event_identifiers;

    // try to lookup provided event, loop over map cells
    while (mcs->name) {
        if (mcs->key == event)
            return result;
        mcs++;
        ++result;
    }

    // event is not found in the provided event database
    // return last element registered as "unknown" (name points to nullptr).
    return result;
}

const char *get_event_name(unsigned int index)
{
    return event_identifiers[index].name;
}

const char * get_event_name(event_t event)
{
    unsigned int index = get_event_index(event);
    return event_identifiers[index].name;
}

const char *get_event_text(unsigned int index)
{
    return event_identifiers[index].text;
}

const char *get_event_text(event_t event)
{
    unsigned int index = get_event_index(event);

    return event_identifiers[index].text;
}

std::string get_event_info(event_t event, std::string format)
{
    std::string res = format;
    unsigned int index = get_event_index(event);
    const char *name_p = get_event_name(index);
    std::string name = name_p ? name_p : std::to_string(static_cast<int>(event));

    size_t pos = 0;
    while ((pos = res.find("%n")) != std::string::npos)
        res.replace(pos, 2, name);
    while ((pos = res.find("%t")) != std::string::npos)
        res.replace(pos, 2, get_event_text(index));
    //std::replace(res.begin(), res.end(), string("%n"), string(get_event_name(index)));
    //std::replace(res.begin(), res.end(), string("%t"), string(get_event_text(index)));
    return res;
}
