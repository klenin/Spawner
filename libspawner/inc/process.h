#ifndef _SPAWNER_PROCESS_H_
#define _SPAWNER_PROCESS_H_

#include <list>
#include <vector>//dd
#include <string>
#include <time.h>//dd
#include "restrictions.h"
#include "processproxy.h"
#include "platform.h"
#include "pipes.h"
#include "status.h"
#include "report.h"
#include "options.h"

using namespace std;

const size_t MAX_RATE_COUNT = 10;

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
class process_class
{
private:
    DWORD process_creation_flags;
    startupinfo_t si;
    COptions options;
    std::string program;
protected:
    CPipe std_input, std_output, std_error;
    process_info_t process_info;
    process_status_t process_status;
    virtual void init_process(char *cmd, const char *wd)
    {
        if ( !CreateProcess(program.c_str(),
            cmd,
            NULL, NULL,
            TRUE,
            process_creation_flags,
            NULL, wd,
            &si, &process_info) )
        {
            if ( !CreateProcess(NULL,
                cmd,
                NULL, NULL,
                TRUE,
                process_creation_flags,
                NULL, wd,
                &si, &process_info) )
            {
                DWORD le = GetLastError();
                process_status = process_failed_to_create;
            }
        }
    }
    void create_process()
    {
        ZeroMemory(&si, sizeof(si));

        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = std_input.ReadPipe();
        si.hStdOutput = std_output.WritePipe();
        si.hStdError = std_error.WritePipe();
        si.lpDesktop = "";
        process_creation_flags = PROCESS_CREATION_FLAGS;

        if (options.hide_gui)
        {
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
        }
        if (options.silent_errors)
            SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        if (options.debug)
            process_creation_flags |= DEBUG_PROCESS;

        // Extracting program name and generating cmd line
        char *cmd;
        const char *wd = (options.working_directory != "")?options.working_directory.c_str():NULL;
        string command_line;
        size_t  index_win = program.find_last_of('\\'), 
            index_nix = program.find_last_of('/');

        if (index_win != string::npos)
            command_line = program.substr(index_win + 1);
        else if (index_nix != string::npos)
            command_line = program.substr(index_nix + 1);
        else
            command_line = program;

        command_line = command_line + " " + (options.string_arguments==""?options.get_arguments():options.string_arguments);
        cmd = new char [command_line.size()+1];
        strcpy(cmd, command_line.c_str());
        init_process(cmd, wd);
        delete[] cmd;
    }
    virtual void free()
    {
    }
public:
    process_class(const std::string &program, const COptions &options):program(program), options(options), std_input(STD_INPUT), std_output(STD_OUTPUT), std_error(STD_ERROR)
    {
        create_process();
    }
    ~process_class()
    {
        std_output.finish();
        std_error.finish();
        CloseHandleSafe(process_info.hProcess);
        CloseHandleSafe(process_info.hThread);
        free();
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
        //cout << process_status << endl;
        if (process_status == process_failed_to_create)
            return process_status;
        /************************* DIRTY HACK *************************
        if (terminate_reason != terminate_reason_not_terminated)
            process_status = process_finished_terminated;
        ************************* END OF HACK ************************/
        if (process_status & process_finished || process_status == process_suspended)
            return process_status;
        unsigned long exitcode = get_exit_code();
        if (exitcode == exit_code_still_active)
            process_status = process_still_active;
        else
            process_status = process_finished_abnormally;
        if (exitcode == 0)
            process_status = process_finished_normal;
        return process_status;
    }
    exception_t get_exception()
    {
        if (get_process_status() == process_finished_abnormally)
            return (exception_t)get_exit_code();
        else return exception_exception_no;
    }

    void run_process()
    {
        if (get_process_status() == process_failed_to_create)
            return;
        ResumeThread(process_info.hThread);
        std_output.bufferize();
        std_error.bufferize();
    }
    handle_t handle()
    {
        return process_info.hProcess;
    }
    void wait(const unsigned long &interval)
    {
        WaitForSingleObject(process_info.hProcess, interval);// TODO: get rid of this
        std_output.wait_for_pipe(100);
        std_error.wait_for_pipe(100);
    }
    std::string get_program() const
    {
        return program;
    }
    COptions get_options() const
    {
        return options;
    }
};

class process_wrapper
{
protected:
    handle_t hIOCP;
    handle_t hJob;
    process_class process;
    CRestrictions restrictions;
    void set_restrictions()
    {
        /* implement restriction value check */
        hJob = CreateJobObject(NULL, NULL);
        DWORD le = GetLastError();

        // Memory and time limit
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
        memset(&joeli, 0, sizeof(joeli));
        joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;

        if (restrictions.get_restriction(restriction_memory_limit) != restriction_no_limit)
        {   
            joeli.JobMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
            joeli.ProcessMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
            joeli.BasicLimitInformation.LimitFlags |=
                JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
        }

        SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli));

        // Security limit
        if (restrictions.get_restriction(restriction_security_limit) != restriction_no_limit)
        {
            JOBOBJECT_BASIC_UI_RESTRICTIONS buir;
#ifdef JOB_OBJECT_UILIMIT_ALL
            buir.UIRestrictionsClass = JOB_OBJECT_UILIMIT_ALL;
#else
            buir.UIRestrictionsClass = 0x000000FF;
#endif
            SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &buir, sizeof(buir));
        }

        // Assigning created process to job object
        AssignProcessToJobObject(hJob, process.handle());

        hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 1, 1);

        JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp; 
        joacp.CompletionKey = (PVOID)COMPLETION_KEY; 
        joacp.CompletionPort = hIOCP; 
        SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
    }
//    CProcess(){};
//
//    string application;
//    string worging_directory;
//    list<string> arguments;
//    //arguments
//    //working directory
//
//    process_info_t process_info;
//    startupinfo_t si;
//
    thread_t check_thread;
//    thread_t thread, check, completition, debug_thread;
//    handle_t process_initiated;
//
//    list<handle_t> threads;
//
//    CPipe std_input, std_output, std_error;
//
//    CRestrictions restrictions;
//    COptions options;
    bool running;
//
    process_status_t process_status;
    terminate_reason_t terminate_reason;
    CReport report;
//
//    static thread_return_t debug_thread_proc(thread_param_t param);
//    static thread_return_t process_completition(thread_param_t param);
    static thread_return_t check_limits_proc(thread_param_t param)
    {
        process_wrapper *self = (process_wrapper*)param;
        CRestrictions restrictions = self->restrictions;
        DWORD t;
        int dt;
        double total_rate = 10000.0;
        std::vector<double> rates;
        rates.push_back(total_rate);
        JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;

        if (restrictions.get_restriction(restriction_processor_time_limit) == restriction_no_limit &&
            restrictions.get_restriction(restriction_user_time_limit) == restriction_no_limit &&
            restrictions.get_restriction(restriction_write_limit) == restriction_no_limit &&
            restrictions.get_restriction(restriction_load_ratio) == restriction_no_limit)
            return 0;
        static const double sec_clocks = (double)1000.0/CLOCKS_PER_SEC;

        t = GetTickCount();
        dt = clock();
        LONGLONG last_quad_part = 0;
        self->report.load_ratio = 10000.0;
        while (1)
        {
            BOOL rs = QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL);
            if (!rs)
            {
                DWORD le = GetLastError();
                std::cout << "!!!!" << le;// TODO: fail
                break;
            }

            if (restrictions.get_restriction(restriction_write_limit) != restriction_no_limit && 
                bai.IoInfo.WriteTransferCount > restrictions.get_restriction(restriction_write_limit))
            {
                PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, NULL);
                break;
            }

            if (restrictions.get_restriction(restriction_processor_time_limit) != restriction_no_limit && 
                (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart > SECOND_COEFF * restrictions.get_restriction(restriction_processor_time_limit))
            {
                PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);
                break;
            }
            if (restrictions.get_restriction(restriction_user_time_limit) != restriction_no_limit && 
                (GetTickCount() - t) > restrictions.get_restriction(restriction_user_time_limit))
            {
                PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT, COMPLETION_KEY, NULL);//freezed
                break;
            }
            if ((clock() - dt)*sec_clocks > 100.0 && bai.BasicInfo.TotalUserTime.QuadPart)
            {
                //change to time limit
                double load_ratio = (double)(bai.BasicInfo.TotalUserTime.QuadPart - last_quad_part)/(sec_clocks*(clock() - dt));
                rates.push_back(load_ratio);// make everything integer
                if (rates.size() >= MAX_RATE_COUNT)
                {
                    rates.pop_back();
                }
                total_rate += load_ratio;
                //self->report.load_ratio
                last_quad_part = bai.BasicInfo.TotalUserTime.QuadPart;
                dt = clock();
                if (restrictions.get_restriction(restriction_load_ratio) != restriction_no_limit)
                {
                    if (self->report.load_ratio < 0.01*self->restrictions.get_restriction(restriction_load_ratio))
                    {
                        PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT, COMPLETION_KEY, NULL);//freezed
                        break;
                    }
                }
            }
            Sleep(1);
        }
        return 0;
    }
    void wait()
    {
        clock_t program_run_time = clock();//TODO:make this global
        if (restrictions.get_restriction(restriction_user_time_limit) == restriction_no_limit)
        {
            //WaitForSingleObject(process_info.hProcess, INFINITE);
            // careful with this comment - it may cause problems with make
        }
        DWORD dwNumBytes, dwKey;
        LPOVERLAPPED completedOverlapped;  
        static CHAR buf[1024];
        //set msg
        int message = 0;
        do
        {     
            GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);

            switch (dwNumBytes)
            {
            case JOB_OBJECT_MSG_NEW_PROCESS:
                break;
            case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
                message++;
                TerminateJobObject(hJob, 0);
                terminate_reason = terminate_reason_time_limit;
                process_status = process_finished_terminated;
                break;
            case JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT:  
                message++;
                TerminateJobObject(hJob, 0);
                terminate_reason = terminate_reason_write_limit;
                process_status = process_finished_terminated;
                break;
            case JOB_OBJECT_MSG_EXIT_PROCESS:
                message++;
                break;
            case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
                message++;
                process_status = process_finished_abnormally;
                break;
            case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
                message++;
                TerminateJobObject(hJob, 0);
                terminate_reason = terminate_reason_memory_limit;
                process_status = process_finished_terminated;
                break;
            case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
                message++;
                TerminateJobObject(hJob, 0);
                terminate_reason = terminate_reason_memory_limit;
                process_status = process_finished_terminated;
                break;
            case JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT:
                message++;
                TerminateJobObject(hJob, 0);
                terminate_reason = terminate_reason_user_time_limit;
                process_status = process_finished_terminated;
                break;
            case JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT:
                message++;
                TerminateJobObject(hJob, 0);
                terminate_reason = terminate_reason_load_ratio_limit;
                process_status = process_finished_terminated;
                break;
            };
            //cout << dwNumBytes;

        } while (!message);
        program_run_time = clock() - program_run_time;
        cout << program_run_time;
        report.user_time = (program_run_time*1000)/CLOCKS_PER_SEC;
        GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);
        process.wait(INFINITE);
        running = false;
    }
//
//    //to fix
//    void createProcess();
//    void setRestrictions();
//    void wait();
//    void finish();
public:
    process_wrapper(const std::string &program, const COptions &options, const CRestrictions &restrictions):process(program, options), restrictions(restrictions)
    {
        if (process.get_process_status() == process_failed_to_create)
            return;
        set_restrictions();
    }
    ~process_wrapper()
    {
        CloseHandleSafe(hIOCP);
        CloseHandleSafe(hJob);
        CloseHandleSafe(check_thread);
    }
    void run_process()
    {
        if (process.get_process_status() == process_failed_to_create)
            return;
        process.run_process();
        check_thread = CreateThread(NULL, 0, check_limits_proc, this, 0, NULL);
        wait();
    }
    terminate_reason_t get_terminate_reason()
    {
        return terminate_reason;
    }
    CReport get_report()
    {
        JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
        report.process_status = process.get_process_status();
        if (hJob == INVALID_HANDLE_VALUE || process.get_process_status() == process_failed_to_create)
            return report;
        if (!QueryInformationJobObject(hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL))
        {
            //throw GetWin32Error("QueryInformationJobObject");
        }

        report.processor_time = bai.BasicInfo.TotalUserTime.QuadPart;
        report.kernel_time = bai.BasicInfo.TotalKernelTime.QuadPart;
        report.write_transfer_count = bai.IoInfo.WriteTransferCount;

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION xli;
        if (!QueryInformationJobObject(hJob, JobObjectExtendedLimitInformation, &xli, sizeof(xli), NULL))
        {
            //throw GetWin32Error("QueryInformationJobObject");
        }

        report.peak_memory_used = xli.PeakJobMemoryUsed;

        report.application_name = process.get_program();

        report.exception = process.get_exception();
        report.terminate_reason = get_terminate_reason();
        report.exit_code = process.get_exit_code();

        report.options = process.get_options();
        report.restrictions = restrictions;

        return report;
    }

//    CProcess(const string &file);
//    void prepare();
//    int run();
//    bool set_terminate_reason(const terminate_reason_t &reason);
//    process_id get_process_id();
//    int Run();
//    void RunAsync();
//    //TODO implement Wait function
//
//    ~CProcess();
//
//    istringstream &stdoutput();
//    istringstream &stderror();
//
//    unsigned long get_exit_code();
//    void set_restrictions(const CRestrictions &Restrictions);
//    void set_options(const COptions &Options);
//    void suspend();
//    void resume();
//    void dump_threads(bool suspend = false);
//    process_status_t get_process_status();
//    bool is_running();
//    exception_t get_exception();
//    terminate_reason_t get_terminate_reason();
//    CReport get_report();
//    void Finish();
//    bool Wait(const unsigned long &ms_time);

};

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

    thread_t thread, check, completition, debug_thread;
    handle_t process_initiated;
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

    static thread_return_t debug_thread_proc(thread_param_t param);
    static thread_return_t process_completition(thread_param_t param);
    static thread_return_t check_limits_proc(thread_param_t param);

    //to fix
    void createProcess();
    void setRestrictions();
    void wait();
    void finish();
public:
	CProcess(const string &file);
    void prepare();
    int run();
    bool set_terminate_reason(const terminate_reason_t &reason);
    process_id get_process_id();
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
    void dump_threads(bool suspend = false);
    process_status_t get_process_status();
    bool is_running();
    exception_t get_exception();
    terminate_reason_t get_terminate_reason();
    CReport get_report();
    void Finish();
    bool Wait(const unsigned long &ms_time);
};

#endif//_SPAWNER_PROCESS_H_
