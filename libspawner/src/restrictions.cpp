#include "restrictions.h"
#include <algorithm>

struct restriction_cell {
    restriction_kind_t restriction;
    const char *name;
    const char *description;
};

const restriction_cell restriction_identifiers[] = {
    {restriction_user_time_limit     , "RESTRICTION_USER_TIME_LIMIT"     , ""},
    {restriction_memory_limit        , "RESTRICTION_MEMORY_LIMIT"        , ""},
    {restriction_processor_time_limit, "RESTRICTION_PROCESSOR_TIME_LIMIT", ""},
    {restriction_security_limit      , "RESTRICTION_SECURITY_LIMIT"      , ""},
    {restriction_write_limit         , "RESTRICTION_WRITE_LIMIT"         , ""},
    {restriction_load_ratio          , "RESTRICTION_LOAD_RATIO"          , ""},
    {restriction_idle_time_limit     , "RESTRICTION_IDLE_TIME_LIMIT"     , ""},
    {restriction_max                 , "RESTRICTION_MAX"                 , ""}
};

restrictions_class::restrictions_class()
{
    for (int i = 0; i < restriction_max; i++)
        restrictions[i] = restriction_no_limit;
}

void restrictions_class::set_restriction(const restriction_kind_t &kind, const restriction_t &value){
    restrictions[kind] = value;
}

void restrictions_class::set_restriction(const std::string &kind, const restriction_t &value){
    restrictions[restrictions_class::restriction_by_name(kind)] = value;
}

restriction_t restrictions_class::get_restriction(const restriction_kind_t &kind) const
{
    return restrictions[kind];
}

restriction_t &restrictions_class::operator[](const restriction_kind_t &kind) {
    return restrictions[kind];
}

restriction_kind_t restrictions_class::restriction_by_name(const std::string &name) {
    std::string str = name;
    std::transform(str.begin(), str.end(),str.begin(), ::toupper);
    for (unsigned i = restriction_user_time_limit; i != restriction_max; i++) {
        if (str == restriction_identifiers[i].name) {
            return static_cast<restriction_kind_t>(i);
        }
    }
    return restriction_max;
}


