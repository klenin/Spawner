#ifndef _SPAWNER_PROCESS_PROXY_H_
#define _SPAWNER_PROCESS_PROXY_H_

#include "platform.h"

class CProcessProxy
{
public:
    void Init();
#ifdef _WIN32
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;		
#endif
};


#endif//_SPAWNER_PROCESS_PROXY_H_
