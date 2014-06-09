#include <securerunner.h>
#include <time.h>
#include <vector>
#ifndef JOB_OBJECT_UILIMIT_ALL
#define JOB_OBJECT_UILIMIT_ALL                      0x000000FF
#endif//JOB_OBJECT_UILIMIT_ALL

#ifndef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE          0x00002000
#endif//JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE

const size_t MAX_RATE_COUNT = 20;

bool secure_runner::create_restrictions() {
    if (get_process_status() == process_spawner_crash)
        return false;

    HANDLE jobHandle = NULL;

    /* implement restriction value check */
    std::string job_name = "Local\\";
    job_name += options.session.hash();
    hJob = CreateJobObject(NULL, job_name.c_str());
    DWORD le = GetLastError();

    // Assigning created process to job object
    le = GetLastError();

    // Memory and time limit
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
    memset(&joeli, 0, sizeof(joeli));
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (restrictions.get_restriction(restriction_memory_limit) != restriction_no_limit)
    {
        joeli.JobMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
        joeli.ProcessMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
        joeli.BasicLimitInformation.LimitFlags |=
            JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
    }

    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli));
    le = GetLastError();

    // Security limit
    if (restrictions.get_restriction(restriction_security_limit) != restriction_no_limit)
    {
        JOBOBJECT_BASIC_UI_RESTRICTIONS buir;
        buir.UIRestrictionsClass = JOB_OBJECT_UILIMIT_ALL;
        SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &buir, sizeof(buir));
        JOBOBJECT_SECURITY_LIMIT_INFORMATION job_sec;
        ZeroMemory(&job_sec, sizeof(job_sec));
        job_sec.SecurityLimitFlags =
            JOB_OBJECT_SECURITY_NO_ADMIN |
            JOB_OBJECT_SECURITY_RESTRICTED_TOKEN;
        if (!SetInformationJobObject(hJob, JobObjectSecurityLimitInformation, &job_sec, sizeof(job_sec)))
        {
        }

    }

    hIOCP = CreateIoCompletionPort(handle_default_value, NULL, 1, 1);
    le = GetLastError();

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp;
    joacp.CompletionKey = (PVOID)COMPLETION_KEY;
    joacp.CompletionPort = hIOCP;
    SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
    le = GetLastError();
    return true;
}

bool secure_runner::apply_restrictions()
{
    //if (!create_restrictions)
    //    return false;

    if (!AssignProcessToJobObject(hJob, process_info.hProcess)) {
        DWORD le = GetLastError();
    }
    return true;
}

void secure_runner::create_process()
{
    runner::create_process();
    create_restrictions();
    //check_thread = CreateThread(NULL, 0, check_limits_proc, this, 0, NULL);// may be move this to wait function
}

thread_return_t secure_runner::process_completition_proc( thread_param_t param )
{
    secure_runner *self = (secure_runner*)param;
    self->wait();
    return 0;
}

thread_return_t secure_runner::check_limits_proc( thread_param_t param )
{
    secure_runner *self = (secure_runner*)param;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
    restrictions_class restrictions = self->restrictions;

    if (restrictions[restriction_processor_time_limit] == restriction_no_limit &&
        restrictions[restriction_user_time_limit] == restriction_no_limit &&
        restrictions[restriction_write_limit] == restriction_no_limit &&
        restrictions[restriction_idle_time_limit] == restriction_no_limit)
        return 0;//may be comment this

    DWORD t;
    int dt;
    double total_rate = 10000.0;
    LONGLONG last_quad_part = 0;
    bool is_idle = false;
    self->report.load_ratio = 10000.0;
    std::vector<double> rates;
    static const double sec_clocks = (double)1000.0/CLOCKS_PER_SEC;
    rates.push_back(total_rate);

    t = GetTickCount();
    dt = clock();

    while (1)
    {
        if (!QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), NULL))
            break;

        if (restrictions.get_restriction(restriction_write_limit) != restriction_no_limit &&
            bai.IoInfo.WriteTransferCount > restrictions.get_restriction(restriction_write_limit))
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, NULL);
            break;
        }

        if (restrictions.get_restriction(restriction_processor_time_limit) != restriction_no_limit &&
            (DOUBLE)bai.BasicInfo.TotalUserTime.QuadPart > SECOND_COEFF * restrictions.get_restriction(restriction_processor_time_limit)) {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, NULL);
            break;
        }
        if (restrictions.get_restriction(restriction_user_time_limit) != restriction_no_limit &&
            (GetTickCount() - t) > restrictions.get_restriction(restriction_user_time_limit)) {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT, COMPLETION_KEY, NULL);//freezed
            break;
        }
        if ((clock() - dt)*sec_clocks > 10.0 && bai.BasicInfo.TotalUserTime.QuadPart) {
            //change to time limit
            double load_ratio = (double)(bai.BasicInfo.TotalUserTime.QuadPart - last_quad_part)/(sec_clocks*(clock() - dt));
            rates.push_back(load_ratio);// make everything integer
            if (rates.size() >= MAX_RATE_COUNT) {
                total_rate -= rates[0];
                rates.erase(rates.begin());
            }
            total_rate += load_ratio;
            if (total_rate < 0.0)
                total_rate = 0.0;
            self->report.load_ratio = total_rate/rates.size();
            last_quad_part = bai.BasicInfo.TotalUserTime.QuadPart;
            dt = clock();
            if (restrictions.get_restriction(restriction_load_ratio) != restriction_no_limit) {
                if (self->report.load_ratio < 0.01*self->restrictions.get_restriction(restriction_load_ratio)) {
                    if (!is_idle && restrictions.get_restriction(restriction_idle_time_limit) != restriction_no_limit) {
                        is_idle = true;
                        t = clock();
                    }
                    if (sec_clocks*(clock() - t) > restrictions.get_restriction(restriction_idle_time_limit)) {
                        PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT, COMPLETION_KEY, NULL);//freezed
                        break;
                    }
                } else {
                    is_idle = false;
                }
            }
        }
        Sleep(1);
    }
    return 0;
}

void secure_runner::dump_threads( bool suspend )
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
    if (h != handle_default_value)
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

void secure_runner::free()
{
    CloseHandleSafe(hIOCP);
    CloseHandleSafe(hJob);
    //CloseHandleSafe(check_thread);
    if (check_thread && check_thread != INVALID_HANDLE_VALUE) {
        TerminateThread(check_thread, 0);
        check_thread = INVALID_HANDLE_VALUE;
    }
}

void secure_runner::wait()
{
    clock_t program_run_time = clock();//TODO:make this global
    DWORD dwNumBytes, dwKey;
    LPOVERLAPPED completedOverlapped;
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
        default:
            break;
        };
    } while (!message);

    program_run_time = clock() - program_run_time;
    report.user_time = (program_run_time*1000)/CLOCKS_PER_SEC;

    GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);

    //runner::wait_for(INFINITE); // delete this
    running = false;
}

secure_runner::secure_runner(const std::string &program, const options_class &options, const restrictions_class &restrictions): 
    runner(program, options), restrictions(restrictions),
    hIOCP(handle_default_value), hJob(handle_default_value), check_thread(handle_default_value)
{
}

secure_runner::~secure_runner()
{
    free();
}

void secure_runner::requisites()
{
    apply_restrictions();

    runner::requisites();

    check_thread = CreateThread(NULL, 0, check_limits_proc, this, 0, NULL);
    //completition = CreateThread(NULL, 0, process_completition_proc, this, 0, NULL);
    //WaitForSingleObject(completition, 100); // TODO fix this
    //create in another thread waiting function
}

terminate_reason_t secure_runner::get_terminate_reason()
{
    return terminate_reason;
}

report_class secure_runner::get_report()
{
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
    report.process_status = get_process_status();
    if (hJob != handle_default_value && get_process_status() != process_spawner_crash)
    {
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
        report.terminate_reason = get_terminate_reason();
    }

    return runner::get_report();
}

restrictions_class secure_runner::get_restrictions() const
{
    return restrictions;
}

process_status_t secure_runner::get_process_status()
{
    if (process_status == process_finished_terminated || process_status == process_suspended)
        return process_status;
    return runner::get_process_status();
}

bool secure_runner::is_running()
{
    if (running)
        return true;
    return (get_process_status() & process_still_active) != 0;
}

void secure_runner::suspend()
{
    if (get_process_status() != process_still_active)
        return;
    dump_threads(true);
    process_status = process_suspended;
    //SuspendThread(process_info.hThread);
}

void secure_runner::resume()
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
    process_status = process_still_active;
    get_process_status();
}
