#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
#include <map>
#include <string>
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"

using namespace std;

// To implement
/* 
    CProcess - one time object
  process initialized with constructor
  and then executed with one of the "Run" functions
  simple Run & RunAsync
*/
/* 
    SetCurrentDirectory
*/
// may be create CProcess and CAsyncProcess
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
	int Run(string file);
    void RunAsync();
    CPipe stdinput, stdoutput, stderror;
	~CProcess();
    bool Completed();

protected:
    map<RESTRICTION_KIND, restriction_t> restrics;
    restriction_t restrictions[RESTRICTION_MAX];
    process_info_t process_info;
    thread_t thread, check, completition;
    handle_t hIOCP;
    handle_t hJob;
    startupinfo_t si;

    string application;
    static thread_return_t process_body(thread_param_t param);
    static thread_return_t check_limits(thread_param_t param);
	CProcessProxy proxy;

    //to fix
    void createProcess();
    void setRestrictions();
    void setupJobObject();
    void wait();
    void finish();
};


#endif//_PROCESS_H_
