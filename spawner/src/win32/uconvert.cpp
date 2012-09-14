#include "uconvert.h"

#include <math.h>
#include <sstream>


struct degree_description
{
    degrees_enum type;
    char *name;
    char *short_name;
    //    double convert_rate;
};

const degree_description degree_descriptions[] = {
    {degree_default,"",         ""},
    {degree_deca,   "deca",     "da"},
    {degree_hecto,  "hecto",    "h"},
    {degree_kilo,   "kilo",     "k"},
    {degree_mega,   "mega",     "M"},
    {degree_giga,   "giga",     "G"},
    {degree_tera,   "tera",     "T"},
    {degree_peta,   "peta",     "P"},
    {degree_deci,   "deci",     "d"},
    {degree_centi,  "centi",    "c"},
    {degree_milli,  "milli",    "m"},
    {degree_micro,  "micro",    "u"},
    {degree_nano,   "nano",     "n"},
    {degree_pico,   "pico",     "p"},
    {degree_femto,  "femto",    "f"},
};

struct unit_description
{
    unit_t type;
    char *name;
    char *short_name;
    double convert_rate;
};

const unit_description unit_descriptions[] = {
    {unit_no_unit,      "",         "",     0.0},
    {unit_memory_byte,  "Byte",     "B",    1.0},
    {unit_memory_bit,   "bit",      "b",    0.125},
    {unit_time_second,  "Second",   "s",    1.0},
    {unit_time_minute,  "Minute",   "m",    60.0},
    {unit_time_hour,    "Hour",     "h",    3600.0},
    {unit_time_day,     "Day",      "d",    86400.0},
};
const unsigned int units_count = 7;
const unsigned int degrees_count = 15;

value_t::value_t( const unit_t &unit_type, const degrees_enum &degree_type ) :unit_type(unit_type), degree_type(degree_type)
{

}

unsigned int get_unit_index(const unit_t &unit)
{
    for (unsigned int i = 0; i < units_count; i++)
        if (unit_descriptions[i].type == unit)
            return i;
    return 0;
}

unsigned int get_degree_index(const degrees_enum &degree)
{
    for (unsigned int i = 0; i < degrees_count; i++)
        if (degree_descriptions[i].type == degree)
            return i;
    return 0;
}

unsigned long convert(const value_t &from, const value_t &to, const unsigned long &val)
{
    return (unsigned long)convert(from, to, (long double)val);
}

long double convert(const value_t &from, const value_t &to, const long double &val)
{
    unsigned int from_unit_index = get_unit_index(from.unit_type), 
        to_unit_index = get_unit_index(to.unit_type);
    unsigned int from_degree_index = get_degree_index(from.degree_type), 
        to_degree_index = get_degree_index(to.degree_type);

    if (from_unit_index == 0 || to_unit_index == 0)
        return 0;// TODO fail
    if ((from.unit_type & unit_time) != (to.unit_type & unit_time) || (from.unit_type & unit_memory) != (to.unit_type & unit_memory))
        return 0;// fail

    double base = 10;
    double p = (int)from.degree_type - (int)to.degree_type;
    double coeff = unit_descriptions[from_unit_index].convert_rate/unit_descriptions[to_unit_index].convert_rate;
    double v = val;

    if (to.unit_type & unit_memory)
    {
        if (to.degree_type != degree_default && ((int)to.degree_type) < ((int)degree_kilo))
            return 0;// fail
        if (from.degree_type != degree_default && ((int)from.degree_type) < ((int)degree_kilo))
            return 0;// fail
        p /= 3.0;
        base = 1024.0;
    }

    if (p != 0.0)
        v = v*pow(base, p);
    if (coeff != 1.0)
        v = v*coeff;

    return v;
}

string convert(const value_t &from, const value_t &to, const long double &val, const char *format, const long double &inf_value)
{
    if (val == inf_value)
        return infinite_string;
    double res = convert(from, to, val);
    unsigned int to_unit_index = get_unit_index(to.unit_type);
    unsigned int to_degree_index = get_degree_index(to.degree_type);
    ostringstream osstream;
    osstream << res;
    for (unsigned int i = 0; i < strlen(format); i++)
    {
        switch (format[i])
        {
        case 'D':
            osstream << degree_descriptions[to_degree_index].name;
            break;
        case 'd':
            osstream << degree_descriptions[to_degree_index].short_name;
            break;
        case 'U':
            osstream << unit_descriptions[to_unit_index].name;
            break;
        case 'u':
            osstream << unit_descriptions[to_unit_index].short_name;
            break;
        default:
            osstream << format[i]; break;

        }
    }
    return osstream.str();
}
unsigned long convert(const value_t &to, const string &val, const unsigned long &default_value)
{
    string v = val;
    value_t from(unit_no_unit);
    if (to.unit_type == unit_no_unit)
        return default_value;
    std::istringstream iss(val);
    long double value = 0;
    iss >> value;
    int index = iss.tellg().seekpos();
    if (index == -1)
        return default_value;
    v = v.substr(index, v.length() - index);
    if (to.unit_type & unit_memory)
    {
        from = value_t(unit_memory_byte, degree_mega);
    }
    else
    {
        from = value_t(unit_time_second);
    }
    double result = abs(convert(from, to, value));
    return (unsigned long)result;
}

