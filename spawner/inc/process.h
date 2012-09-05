#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <list>
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

    list<handle_t> threads;

    string application;
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
    void SetRestrictionForKind(restriction_kind_t kind, restriction_t value);
    restriction_t GetRestrictionValue(restriction_kind_t kind);
	void SetArguments(); // ?!
	virtual int Run();
    void RunAsync();

	~CAsyncProcess();
    //bool Completed();

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
    void suspend()
    {
        if (get_process_status() != process_still_active)
            return;
        dumpThreads(true);
        proc_status = process_suspended;
        //SuspendThread(process_info.hThread);
    }
    void resume()
    {
        if (get_process_status() != process_suspended)
            return;
        while (!threads.empty())
        {
            handle_t handle = threads.front();
            threads.pop_front();
            ResumeThread(handle);
            CloseHandle(handle);
        }
        proc_status = process_still_active;
        get_process_status();
    }
    void dumpThreads(bool suspend = false)
    {
        //if process is active and started!!!
        if (!is_running())
            return;
        //while (threads.empty())
        //{
            //CloseHandle(threads.begin()
        //}
        threads.clear();
        HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (h != INVALID_HANDLE_VALUE)
        {
            THREADENTRY32 te;
            te.dwSize = sizeof(te);
            if (Thread32First(h, &te)) 
            {
                do {
                    if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
                        sizeof(te.th32OwnerProcessID) && te.th32OwnerProcessID == process_info.dwProcessId) 
                    {
                        handle_t handle = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if (suspend)
                            SuspendThread(handle);
                        //may be close here??
                        threads.push_back(handle);
                        /*printf("Process 0x%04x Thread 0x%04x\n",
                                te.th32OwnerProcessID, te.th32ThreadID);*/
                    }
                    te.dwSize = sizeof(te);
                } while (Thread32Next(h, &te));
            }
            CloseHandle(h);
        }
    }
    process_status_t get_process_status()
    {
        // renew process status
        if (proc_status & process_finished || proc_status == process_suspended)
            return proc_status;
        unsigned long exitcode = get_exit_code();
        if (exitcode == exit_code_still_active)
            proc_status = process_still_active;
        else
            proc_status = process_finished_abnormally;
        if (exitcode == 0)
            proc_status = process_finished_normal;
        return proc_status;
    }
    bool is_running()
    {
        return (bool)(get_process_status() & process_is_active);
    }
    exception_t get_exception()
    {
        if (get_process_status() == process_finished_abnormally)
            return (exception_t)get_exit_code();
        else return exception_no_exception;
    }
    void WaitAndFinish()
    {
        finish();
    }
};

class CSimpleProcess: public CAsyncProcess
{
public:
    CSimpleProcess(string file);
    int Run();
};


#endif//_PROCESS_H_
