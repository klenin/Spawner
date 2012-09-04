#ifndef _RESTRICTIONS_H_
#define _RESTRICTIONS_H_

typedef enum
{
    restriction_user_time_limit         = 0x0,
    restriction_memory_limit            = 0x1,
    restriction_processor_time_limit    = 0x2,
    restriction_security_limit          = 0x3,
    restriction_write_limit             = 0x4,
    restriction_gui_limit               = 0x5,//not restriction, but an option
    restriction_max                     = 0x6
} restriction_kind_t;

typedef enum
{

} option_t;

typedef unsigned int restriction_t;


const restriction_t restriction_no_limit = 0xffffffff;
const restriction_t RESTRICTION_LIMITED  = 0x00000001;


#endif//_RESTRICTIONS_H_
