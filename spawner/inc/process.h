#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
#include <string>
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"
#include "status.h"
#include "report.h"

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
protected:
    CAsyncProcess(){};

    string application;
    //arguments
    //working directory

    process_info_t process_info;
    startupinfo_t si;

    thread_t thread, check, completition;
    handle_t hIOCP;
    handle_t hJob;

    list<handle_t> threads;

    CPipe std_input, std_output, std_error;

    CRestrictions restrictions;

    process_status_t process_status;
    terminate_reason_t terminate_reason;
    CReport report;

    static thread_return_t process_completition(thread_param_t param);
    static thread_return_t check_limits(thread_param_t param);

    //to fix
    void createProcess();
    void setRestrictions();
    void setupJobObject();
    void wait();
    void finish();
public:
	CAsyncProcess(string file);
	void SetArguments(); // ?!
	int Run();
    void RunAsync();
    //TODO implement Wait function

	~CAsyncProcess();

    istringstream &stdoutput();
    istringstream &stderror();

    unsigned long get_exit_code();
    void set_restrictions(const CRestrictions &Restrictions);
    void suspend();
    void resume();
    void dumpThreads(bool suspend = false);
    process_status_t get_process_status();
    bool is_running();
    exception_t get_exception();
    terminate_reason_t get_terminate_reason();
    CReport get_report();
    void Finish();
};

class CSimpleProcess: public CAsyncProcess
{
public:
    CSimpleProcess(string file);
    int Run();
};


#endif//_PROCESS_H_
