#ifndef _SPAWNER_H_
#define _SPAWNER_H_

#include <inc/platform.h>
#include <inc/process.h>
#include <inc/pipes.h>
#include <inc/uconvert.h>
#include <inc/report.h>
#include <string>
#include <map>

/*class spawner
{
protected:
    static spawner _instance;
    spawner();
    ~spawner();
    thread_t debug_thread;
    static thread_return_t debug(thread_param_t param);
    typedef std::map<process_id, CProcess*> process_map;

    process_map processes;
public:
    static spawner &instance();
    CProcess *spawn_process(const std::string &program_name, 
        const CRestrictions &restrictions = CRestrictions(), const COptions &options = COptions());
};*/


#endif//_SPAWNER_H_