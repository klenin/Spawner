#ifndef _SPAWNER_UNIT_CONVERT_H_
#define _SPAWNER_UNIT_CONVERT_H_

#include <string>

enum degrees_enum
{
    degree_default  = 0x0,
    degree_deca     = 0x1,
    degree_hecto    = 0x2,
    degree_kilo     = 0x3,
    degree_mega     = 0x6,
    degree_giga     = 0x9,
    degree_tera     = 0xc,
    degree_peta     = 0xf,
//  degree_exa      = 0x12,
//  degree_zetta    = 0x15,
//  degree_yotta    = 0x18,
    degree_deci     = -0x1,
    degree_centi    = -0x2,
    degree_milli    = -0x3,
    degree_m4       = -0x4,
    degree_micro    = -0x6,
    degree_nano     = -0x9,
    degree_pico     = -0xc,
    degree_femto    = -0xf,
//  degree_deci     = -0x1,
//  degree_deci     = -0x1,
//  degree_deci     = -0x1,

};

enum unit_t
{
    unit_no_unit        = 0x0,
    unit_memory_byte    = 0x1,
    unit_memory_bit     = 0x5,
    unit_time_second    = 0x2,
    unit_time_minute    = 0x6,
    unit_time_hour      = 0xa,
    unit_time_day       = 0xe,
};

const unsigned int unit_memory  = 0x1;
const unsigned int unit_time    = 0x2;

struct value_t
{
    value_t(const unit_t &unit_type, const degrees_enum &degree_type = degree_default);
    unit_t unit_type;
    degrees_enum degree_type;
};

unsigned long convert(const value_t &from, const value_t &to, const unsigned long &val);
long double convert(const value_t &from, const value_t &to, const long double &val);
std::string convert(const value_t &from, const value_t &to, const long double &val, const char *format, const long double &inf_value = -1);

unsigned long convert(const value_t &to, const std::string &val, const unsigned long &default_value = 0);

#endif//_SPAWNER_UNIT_CONVERT_H_
