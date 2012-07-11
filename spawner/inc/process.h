#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
#include <map>
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"

class CProcess
{
public:
	CProcess();
    void SetRestrictionForKind(RESTRICTION_KIND kind, restriction_t value)
    {
        restrictions[kind] = value;
    }
    restriction_t GetRestrictionValue(RESTRICTION_KIND kind)
    {
        return restrictions[kind];
    }
	void SetArguments(); // ?!
	int Run(char *argv[]);
    void RunAsync();
    CPipe stdinput, stdoutput, stderror;
	~CProcess();
    HANDLE hIOCP;
    HANDLE hJob;
protected:
    restriction_t restrictions[RESTRICTION_MAX];
    process_info_t process_info;
    thread_t thread;
    static thread_return_t process_body(thread_param_t param);
    static thread_return_t read_body(thread_param_t param);
    static thread_return_t check_limits(thread_param_t param);
	CProcessProxy proxy;
};


#endif//_PROCESS_H_
