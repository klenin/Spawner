#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
#include <map>
#include <string>
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"
#include "status.h"

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
class CAsyncProcess
{
public:
	CAsyncProcess(string file);
    void SetRestrictionForKind(restriction_kind_t kind, restriction_t value)
    {
        restrictions[kind] = value;
    }
    restriction_t GetRestrictionValue(restriction_kind_t kind)
    {
        return restrictions[kind];
    }
	void SetArguments(); // ?!
	virtual int Run();
    void RunAsync();

	~CAsyncProcess();
    bool Completed();

    istringstream &stdoutput()
    {
        return std_output.stream();
    }

    istringstream &stderror()
    {
        return std_error.stream();
    }
    unsigned long get_exit_code()
    {
        DWORD dwExitCode = 0;
        if (!GetExitCodeProcess(process_info.hProcess, &dwExitCode))
            throw "!!!";
        return dwExitCode;
    }
    process_status_t get_process_status()
    {
        // renew process status
        if (proc_status & process_finished)
            return proc_status;
        unsigned long exitcode = get_exit_code();
        if (exitcode != exit_code_still_active)
            proc_status = process_finished_abnormally;
        if (exitcode == 0)
            proc_status = process_finished_normal;
        return proc_status;
    }
    bool is_running()
    {
        return get_exit_code() == exit_code_still_active;
    }
    exception_t get_exception()
    {
        if (get_process_status() == process_finished_abnormally)
            return (exception_t)get_exit_code();
        else return exception_no_exception;
    }
    void WaitAndFinish()
    {
        wait();
        finish();
    }
protected:
    CAsyncProcess(){};
    restriction_t restrictions[restriction_max];
    process_info_t process_info;
    thread_t thread, check, completition;
    handle_t hIOCP;
    handle_t hJob;
    startupinfo_t si;
    process_status_t proc_status;
    CPipe std_input, std_output, std_error;

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

class CSimpleProcess: public CAsyncProcess
{
public:
    CSimpleProcess(string file);
    int Run();
};


#endif//_PROCESS_H_
