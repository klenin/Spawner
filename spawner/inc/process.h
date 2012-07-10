#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"

class CProcess
{
public:
	CProcess();
	void SetRestrictions(CRestriction *restriction)
	{
		if (!restriction)
			return;
		restrictions.push_back(restriction);
	}
	void SetArguments(); // ?!
	int Run(char *argv[]);
    void RunAsync();
    CPipe stdinput, stdoutput, stderror;
	~CProcess();
    PROCESS_INFORMATION ProcessInformation(){return pi;}
    void SetProcessInformation(PROCESS_INFORMATION p){pi = p;}
protected:
    PROCESS_INFORMATION pi;
	void apply_restrictions()
	{
        proxy.Init();
		for (std::list<CRestriction*>::iterator i = restrictions.begin();
			i != restrictions.end(); ++i)
		{
			(*i)->ApplyRestriction(proxy);
		}
	}
    thread_t thread;
    static thread_return_t process_body(thread_param_t param);
    static thread_return_t read_body(thread_param_t param);
	std::list<CRestriction*> restrictions;
	CProcessProxy proxy;
};


#endif//_PROCESS_H_
