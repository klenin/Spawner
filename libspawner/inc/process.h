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
#include "options.h"

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
protected:
    CProcess(){};

    string application;
    string worging_directory;
    list<string> arguments;
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
    COptions options;
    bool running;

    process_status_t process_status;
    terminate_reason_t terminate_reason;
    CReport report;

    static thread_return_t process_completition(thread_param_t param);
    static thread_return_t check_limits(thread_param_t param);

    //to fix
    void createProcess();
    void setRestrictions();
    void wait();
    void finish();
public:
	CProcess(string file);
	void SetArguments(); // ?!
	int Run();
    void RunAsync();
    //TODO implement Wait function

	~CProcess();

    istringstream &stdoutput();
    istringstream &stderror();

    unsigned long get_exit_code();
    void set_restrictions(const CRestrictions &Restrictions);
    void set_options(const COptions &Options);
    void suspend();
    void resume();
    void dumpThreads(bool suspend = false);
    process_status_t get_process_status();
    bool is_running();
    exception_t get_exception();
    terminate_reason_t get_terminate_reason();
    CReport get_report();
    void Finish();
    bool Wait(const unsigned long &ms_time);
};

#endif//_PROCESS_H_
