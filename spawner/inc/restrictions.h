#ifndef _RESTRICTIONS_H_
#define _RESTRICTIONS_H_

typedef enum
{
	RESTRICTION_USER_TIME_LIMIT         = 0x0,
	RESTRICTION_MEMORY_LIMIT            = 0x1,
	RESTRICTION_PROCESSOR_TIME_LIMIT    = 0x2,
	RESTRICTION_SECURITY_LIMIT          = 0x3,
    RESTRICTION_WRITE_LIMIT             = 0x4,
    RESTRICTION_GUI_LIMIT               = 0x5,
    RESTRICTION_MAX                     = 0x6

} RESTRICTION_KIND;

typedef unsigned int restriction_t;


const restriction_t RESTRICTION_NO_LIMIT = 0xffffffff;
const restriction_t RESTRICTION_LIMITED  = 0x00000001;


#endif//_RESTRICTIONS_H_
