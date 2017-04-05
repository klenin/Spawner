#include "uconvert.h"

#include <math.h>
#include <sstream>
#include <string.h>
#include <stdlib.h>

const char infinite_string[] = "Infinity";
const unsigned int DEFAULT_PRECISION = 6;

struct degree_description
{
    degrees_enum type;
    const char *name;
    const char *short_name;
    //    double convert_rate;
};

const degree_description degree_descriptions[] = {
    {degree_default,"",         ""},
    {degree_deca,   "deca",     "da"},
    {degree_centi,  "percent",  "%"},
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
    const char *name;
    const char *short_name;
    double convert_rate;
};

const unit_description unit_descriptions[] = {
    {unit_no_unit,      "",         "",     1.0},
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
    return units_count;
}

unsigned int get_degree_index(const degrees_enum &degree)
{
    for (unsigned int i = 0; i < degrees_count; i++)
        if (degree_descriptions[i].type == degree)
            return i;
    return degrees_count;
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

    if ((from.unit_type & unit_time) != (to.unit_type & unit_time) || (from.unit_type & unit_memory) != (to.unit_type & unit_memory))
        return 0.0;// fail

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

std::string unit_short_name(const unit_t &unit_type) {
    return unit_descriptions[get_unit_index(unit_type)].short_name;
}

std::string unit_name(const unit_t &unit_type) {
    return unit_descriptions[get_unit_index(unit_type)].name;
}

std::string degree_short_name(const degrees_enum &degree_type) {
    return degree_descriptions[get_degree_index(degree_type)].short_name;
}

std::string degree_name(const degrees_enum &degree_type) {
    return degree_descriptions[get_degree_index(degree_type)].name;
}

std::string convert(const value_t &from, const value_t &to, const long double &val, const char *format, const long double &inf_value)
{
    if (val == inf_value)
        return infinite_string;
    double res = convert(from, to, val);
    unsigned int to_unit_index = get_unit_index(to.unit_type);
    unsigned int to_degree_index = get_degree_index(to.degree_type);
    std::ostringstream osstream;
    osstream.setf(std::ios::fixed,std::ios::floatfield);
    osstream.precision(DEFAULT_PRECISION);
    osstream << res;
    for (unsigned int i = 0; format[i] != '\0'; i++)
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

// Reads value from string val and tries to convert it to to's type
// If there were some errors - function returns default_value
// Default from values for different types are
// * memory - MB
// * time - s
unsigned long convert(const value_t &to, const std::string &val, const unsigned long &default_value)
{
    std::string v = val;
    value_t from(unit_no_unit);
    long double value = 0.0;

    std::istringstream iss(val);

    if (!(iss >> value)) {
        throw std::string("Number expected");
        return default_value;
    }

    std::streamoff current_index = iss.tellg();

    if (current_index >= 0) {
        v = v.substr((size_t)(current_index), v.length() - (size_t)current_index);
    } else {
        v.clear();
    }

    if (to.unit_type & unit_memory) {
        from = value_t(unit_memory_byte, degree_mega);
    }
    if (to.unit_type & unit_time) {
        from = value_t(unit_time_second);
    }

    if (v.length() > 0) {
        size_t len = v.length(), index = 0, degree_index = 0, unit_index = 0;
        for (degree_index = 1; degree_index < degrees_count; degree_index++) {
            while (index < len && index < strlen(degree_descriptions[degree_index].short_name) &&
                v[index]==degree_descriptions[degree_index].short_name[index]) {
                index++;
            }
            if (index == strlen(degree_descriptions[degree_index].short_name)) {
                break;
            }
            index = 0;
        }
        int old_index = index;
        for (unit_index = 1; unit_index < units_count; unit_index++) {
            while (index < len && index-old_index < strlen(unit_descriptions[unit_index].short_name) &&
                v[index]==unit_descriptions[unit_index].short_name[index - old_index]) {
                index++;
            }
            if (index == (old_index + strlen(unit_descriptions[unit_index].short_name))) {
                break;
            }
            index = old_index;
        }
        if (v.length() == 1) {
            if (degree_index == degrees_count && unit_index != units_count) {
                degree_index = 0;
            } else if (degree_index != degrees_count && unit_index == units_count) {
                from.unit_type = to.unit_type;
                unit_index = get_unit_index(from.unit_type);
            }
        }
        if (degree_index < degrees_count) {
            from.degree_type = degree_descriptions[degree_index].type;
        }
        if (unit_index < units_count) {
            from.unit_type = unit_descriptions[unit_index].type;
        }
        if (!((to.unit_type & from.unit_type) & unit_memory || (to.unit_type & from.unit_type) & unit_time || (from.unit_type == to.unit_type && from.unit_type == unit_no_unit)) ||
            unit_index == units_count || degree_index == degrees_count) {
            throw std::string("Incompatible unit types");
            return default_value;
        }
    }

    unsigned long result = labs((long)convert(from, to, value));

    return result;
}

