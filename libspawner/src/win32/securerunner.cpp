#include "securerunner.h"

#include <ctime>
#include <vector>

#include <WinBase.h>
#include <ImageHlp.h>

#include "inc/error.h"

// these aren't defined in Windows headers from mingw, so here it is

#ifndef JOB_OBJECT_UILIMIT_ALL
#define JOB_OBJECT_UILIMIT_ALL 0x000000FF
#endif

#ifndef JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE 0x00002000
#endif

const size_t MAX_RATE_COUNT = 20;

bool secure_runner::try_handle_createproc_error() {
    const DWORD error_code = GetLastError();
    bool segments_exceed_limit = false;
    
    if (restrictions.check_restriction(restriction_memory_limit)) {
        LOADED_IMAGE exe_image;
        char* exe_path_rw = _strdup(program.c_str());

        if (MapAndLoad(exe_path_rw, nullptr, &exe_image, FALSE, TRUE)) {
            const restriction_t proc_memory_limit = restrictions.get_restriction(restriction_memory_limit);
            const DWORD stack_segment_size = exe_image.FileHeader->OptionalHeader.SizeOfStackReserve;
            const DWORD data_segment_size = exe_image.FileHeader->OptionalHeader.SizeOfUninitializedData;
            segments_exceed_limit = proc_memory_limit < (stack_segment_size + data_segment_size);
            UnMapAndLoad(&exe_image);
        }

        std::free(exe_path_rw);

        // MapAndLoad() / UnMapAndLoad() reset Windows last error, so we restore it
        SetLastError(error_code);
    }

    const bool stackseg_related_error =
        (error_code == ERROR_NOT_ENOUGH_MEMORY) ||
        (error_code == ERROR_INVALID_PARAMETER); // Windows XP specific

    const bool dataseg_related_error =
        (error_code == ERROR_INVALID_PARAMETER) ||
        (error_code == ERROR_BAD_EXE_FORMAT);

    if (segments_exceed_limit && (stackseg_related_error || dataseg_related_error)) {
        terminate_reason = terminate_reason_memory_limit;
        return true;
    }

    return false;
}

bool secure_runner::create_restrictions() {
    if (get_process_status() == process_spawner_crash)
        return false;

    HANDLE jobHandle = nullptr;

    /* implement restriction value check */
    std::string job_name = "Local\\";
    job_name += options.session.hash();
    hJob = CreateJobObject(nullptr, job_name.c_str());
    DWORD le = GetLastError();

    // Assigning created process to job object
    le = GetLastError();

    // Memory and time limit
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION joeli;
    memset(&joeli, 0, sizeof(joeli));
    joeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (restrictions.check_restriction(restriction_memory_limit))
    {
        joeli.JobMemoryLimit = restrictions.get_restriction(restriction_memory_limit);
        joeli.ProcessMemoryLimit = joeli.JobMemoryLimit;
        joeli.BasicLimitInformation.LimitFlags |=
            JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
    }

    SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &joeli, sizeof(joeli));
    le = GetLastError();

    // Security limit
    if (restrictions.check_restriction(restriction_security_limit))
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

    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 1, 1);
    le = GetLastError();

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT joacp;
    joacp.CompletionKey = (PVOID)COMPLETION_KEY;
    joacp.CompletionPort = hIOCP;
    SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &joacp, sizeof(joacp));
    le = GetLastError();
    return true;
}

void secure_runner::apply_restrictions()
{
    if (AssignProcessToJobObject(hJob, process_info.hProcess) == 0) {
        PANIC(get_win_last_error_string());
    }
}

void secure_runner::create_process()
{
    runner::create_process();
    create_restrictions();
}

thread_return_t secure_runner::check_limits_proc( thread_param_t param )
{
    secure_runner *self = (secure_runner*)param;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;

    double total_rate = 10000.0;
    LONGLONG last_quad_part = 0;
    bool is_idle = false;
    self->report.load_ratio = 0.0;
    static const double sec_clocks = (double)1000.0/CLOCKS_PER_SEC;

    unsigned long long idle_dt = self->get_time_since_create();
    unsigned long long dt = 0, idle_time = 0;

    while (1) {
        restrictions_class restrictions = self->restrictions;

        if (!QueryInformationJobObject(self->hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), nullptr))
            break;

        if (bai.BasicInfo.ActiveProcesses == 0)
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_EXIT_PROCESS, COMPLETION_KEY, nullptr);
            break;
        }

        if (restrictions.check_restriction(restriction_write_limit) &&
            bai.IoInfo.WriteTransferCount > restrictions.get_restriction(restriction_write_limit)) {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT, COMPLETION_KEY, nullptr);
            break;
        }

        if (self->prolong_time_limits_
         || self->get_process_status_no_side_effects() == process_suspended) {
            self->base_time_processor_ = bai.BasicInfo.TotalUserTime.QuadPart;
            self->base_time_user_ = self->get_time_since_create();
            self->prolong_time_limits_ = false;
        }

        if (restrictions.check_restriction(restriction_processor_time_limit) &&
            (DOUBLE)(bai.BasicInfo.TotalUserTime.QuadPart - self->base_time_processor_) >
            10 * restrictions.get_restriction(restriction_processor_time_limit)) {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_END_OF_PROCESS_TIME, COMPLETION_KEY, nullptr);
            break;
        }
        if (restrictions.check_restriction(restriction_user_time_limit) &&
            (self->get_time_since_create() - self->base_time_user_) >
            10 * restrictions.get_restriction(restriction_user_time_limit)) {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT, COMPLETION_KEY, nullptr);//freezed
            break;
        }
        if ((dt=self->get_time_since_create() - idle_dt) > 100000) {//10ms
            idle_dt = self->get_time_since_create();

            double load_ratio = 10000.0*(double)(bai.BasicInfo.TotalUserTime.QuadPart - last_quad_part)/dt;
            if (load_ratio < 0.0) {
                load_ratio = 0.0;
            }
            self->report.load_ratio = self->report.load_ratio*0.95 + 0.05*load_ratio;

            last_quad_part = bai.BasicInfo.TotalUserTime.QuadPart;
            if (restrictions.check_restriction(restriction_load_ratio)) {
                if (self->report.load_ratio < self->restrictions.get_restriction(restriction_load_ratio)) {
                    if (!is_idle && restrictions.check_restriction(restriction_idle_time_limit)) {
                        is_idle = true;
                        idle_time = idle_dt;
                    }
                    if ((self->get_time_since_create() - idle_time) > 10*restrictions.get_restriction(restriction_idle_time_limit)) {
                        PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT, COMPLETION_KEY, nullptr);//freezed
                        break;
                    }
                } else {
                    is_idle = false;
                }
            }
        }
        if (
            restrictions.check_restriction(restriction_processes_count_limit)
            && bai.BasicInfo.TotalProcesses > restrictions.get_restriction(restriction_processes_count_limit)
            )
        {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_COUNT_LIMIT, COMPLETION_KEY, nullptr);
            break;
        }
        if (self->force_stop) {
            PostQueuedCompletionStatus(self->hIOCP, JOB_OBJECT_MSG_PROCESS_CONTROLLER_STOP, COMPLETION_KEY, nullptr);
            break;
        }
        Sleep(1);
    }
    return 0;
}

void secure_runner::free()
{
    runner::free();
    CloseHandleSafe(hIOCP);
    CloseHandleSafe(hJob);
    if (check_thread && check_thread != INVALID_HANDLE_VALUE) {
        check_thread = INVALID_HANDLE_VALUE;
    }
}

void secure_runner::wait()
{
    DWORD dwNumBytes;
#ifdef _WIN64
    ULONG_PTR dwKey;
#else
    DWORD dwKey;
#endif
    LPOVERLAPPED completedOverlapped;
    bool waiting;
    bool postLoopWaiting = true;
    bool terminate = false;
    do
    {
        waiting = false;

        GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);

        switch (dwNumBytes)
        {
        case JOB_OBJECT_MSG_END_OF_PROCESS_TIME:
            terminate_reason = terminate_reason_time_limit;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT:
            terminate_reason = terminate_reason_write_limit;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_EXIT_PROCESS:
            postLoopWaiting = false;
            break;
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
            process_status = process_finished_abnormally;
            terminate_reason = terminate_reason_abnormal_exit_process;
            break;
        case JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT:
            terminate_reason = terminate_reason_memory_limit;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
            terminate_reason = terminate_reason_memory_limit;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT:
            terminate_reason = terminate_reason_user_time_limit;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT:
            terminate_reason = terminate_reason_load_ratio_limit;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_PROCESS_COUNT_LIMIT:
            terminate_reason = terminate_reason_created_process;
            terminate = true;
            break;
        case JOB_OBJECT_MSG_PROCESS_CONTROLLER_STOP:
            terminate_reason = terminate_reason_by_controller;
            terminate = true;
            break;
        default:
            waiting = true;
            break;
        };
    } while (waiting);

    if (on_terminate) {
        on_terminate();
    }

    if (terminate) {
        process_status = process_finished_terminated;
        TerminateJobObject(hJob, 0);
    }

    if (postLoopWaiting)
    {
        GetQueuedCompletionStatus(hIOCP, &dwNumBytes, &dwKey, &completedOverlapped, INFINITE);
    }
    //WaitForSingleObject(process_info.hProcess, infinite);
    report.user_time = get_time_since_create() / 10;
    running = false;
}

secure_runner::secure_runner(const std::string &program,
    const options_class &options, const restrictions_class &restrictions)
    : runner(program, options)
    , start_restrictions(restrictions)
    , restrictions(restrictions)
    , hIOCP(INVALID_HANDLE_VALUE)
    , hJob(INVALID_HANDLE_VALUE)
    , check_thread(INVALID_HANDLE_VALUE) {
}

secure_runner::~secure_runner()
{
    CloseHandleSafe(hIOCP);
    CloseHandleSafe(hJob);
    if (check_thread && check_thread != INVALID_HANDLE_VALUE) {
        check_thread = INVALID_HANDLE_VALUE;
    }
}

void secure_runner::requisites()
{
    apply_restrictions();
    runner::requisites();

    check_thread = CreateThread(nullptr, 0, check_limits_proc, this, 0, nullptr);
}

report_class secure_runner::get_report() {
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION bai;
    if (hJob != INVALID_HANDLE_VALUE) {
        if (!QueryInformationJobObject(hJob, JobObjectBasicAndIoAccountingInformation, &bai, sizeof(bai), nullptr)) {
            //throw GetWin32Error("QueryInformationJobObject");
        }

        report.processor_time = bai.BasicInfo.TotalUserTime.QuadPart/10;
        report.kernel_time = bai.BasicInfo.TotalKernelTime.QuadPart/10;
        report.write_transfer_count = bai.IoInfo.WriteTransferCount;

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION xli;
        if (!QueryInformationJobObject(hJob, JobObjectExtendedLimitInformation, &xli, sizeof(xli), nullptr)) {
            //throw GetWin32Error("QueryInformationJobObject");
        }

        report.peak_memory_used = xli.PeakJobMemoryUsed;
    }

    return runner::get_report();
}

restrictions_class secure_runner::get_restrictions() const
{
    return restrictions;
}

process_status_t secure_runner::get_process_status()
{
    if (process_status == process_finished_terminated
     || process_status == process_suspended)
        return process_status;
    return runner::get_process_status();
}

void secure_runner::prolong_time_limits() {
    prolong_time_limits_ = true;
}
