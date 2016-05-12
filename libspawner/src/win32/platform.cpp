#include "inc/uconvert.h"
#include "inc/error.h"
#include "compatibility.h"
#include "platform.h"

#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <string>

// windows8 requires Processenv.h for GetEnvironmentVariableA()
#include <WinBase.h>
#include <Windows.h>
#if defined(_MSC_VER)
#include <dbghelp.h>
#endif

#include "stack_walker.h"


#ifdef OPEN_JOB_OBJECT_DYNAMIC_LOAD
void load_open_job_object() {
    HINSTANCE hDLL = LoadLibrary("kernel32.dll");
    if (!hDLL) {
        //everything failed
    }
    OpenJobObjectA = (OPEN_JOB_OBJECT)GetProcAddress(hDLL, "OpenJobObjectA");
    FreeLibrary(hDLL);
}
#endif//OPEN_JOB_OBJECT_DYNAMIC_LOAD

void CloseHandleSafe_debug(HANDLE &handle, char *file, unsigned int line)
{
    try {
        if (handle == handle_default_value || handle == NULL)
            return;
        CloseHandle(handle);
    } catch (...) {
        std::ofstream log_file("C:\\CATS\\cats-judge\\log.log", std::ofstream::app);
        log_file << file << ":" << line << " " << handle << std::endl;
    }
    handle = handle_default_value;
}

void CloseHandleSafe_real(HANDLE &handle)
{
    if (handle == handle_default_value || handle == NULL)
        return;
    CloseHandle(handle);
    handle = handle_default_value;
}

typedef BOOL(WINAPI *CancelSynchronousIo_func_type)(_In_ HANDLE);
CancelSynchronousIo_func_type CancelSynchronousIo_dyn = nullptr;

void platform_init()
{
    HINSTANCE hKernel32 = LoadLibrary("kernel32.dll");

    if (!hKernel32) {
        return;
    }

    CancelSynchronousIo_dyn = (CancelSynchronousIo_func_type)GetProcAddress(hKernel32, "CancelSynchronousIo");
}

BOOL WINAPI CancelSynchronousIo_wrapper(_In_ HANDLE handle)
{
    if (CancelSynchronousIo_dyn != nullptr) {
        return CancelSynchronousIo_dyn(handle);
    }
    return FALSE;
}

int get_spawner_pid()
{
    return (int) GetCurrentProcessId();
}

void push_shm_report(const char *shm_name, const std::string &report)
{
    HANDLE hOut = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name);
    LPCSTR pRep = (LPTSTR)MapViewOfFile(hOut, FILE_MAP_ALL_ACCESS, 0, 0, options_class::SHARED_MEMORY_BUF_SIZE);

    memcpy((PVOID)pRep, report.c_str(), sizeof(char) * report.length());

    UnmapViewOfFile(pRep);

    CloseHandle(hOut);
}

void pull_shm_report(const char *shm_name, std::string &report)
{
    HANDLE hIn = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        shm_name 
    );

    LPTSTR pRep = (LPTSTR)MapViewOfFile(
        hIn,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        options_class::SHARED_MEMORY_BUF_SIZE
    );

    report = pRep;

    UnmapViewOfFile(pRep);

    CloseHandle(hIn);
}

size_t get_env_var(const char *name, char *buff, size_t size)
{
    return GetEnvironmentVariableA(name, buff, size);
}

std::string ExtractExitStatus(const report_class &rep) {
    unsigned int code = rep.exit_code;

    switch (code) {
        case STATUS_ACCESS_VIOLATION:
            return "AccessViolation";
        case STATUS_ARRAY_BOUNDS_EXCEEDED:
            return "ArrayBoundsExceeded";
        case STATUS_BREAKPOINT:
            return "Breakpoint";
        case STATUS_CONTROL_C_EXIT:
            return "Control_C_Exit";
        case STATUS_DATATYPE_MISALIGNMENT:
            return "DatatypeMisalignment";
        case STATUS_FLOAT_DENORMAL_OPERAND:
            return "FloatDenormalOperand";
        case STATUS_FLOAT_INEXACT_RESULT:
            return "FloatInexactResult";
        case STATUS_FLOAT_INVALID_OPERATION:
            return "FloatInvalidOperation";
        case STATUS_FLOAT_MULTIPLE_FAULTS:
            return "FloatMultipleFaults";
        case STATUS_FLOAT_MULTIPLE_TRAPS:
            return "FloatMultipleTraps";
        case STATUS_FLOAT_OVERFLOW:
            return "FloatOverflow";
        case STATUS_FLOAT_STACK_CHECK:
            return "FloatStackCheck";
        case STATUS_FLOAT_UNDERFLOW:
            return "FloatUnderflow";
        case STATUS_GUARD_PAGE_VIOLATION:
            return "GuardPageViolation";
        case STATUS_ILLEGAL_INSTRUCTION:
            return "IllegalInstruction";
        case STATUS_IN_PAGE_ERROR:
            return "InPageError";
        case STATUS_INVALID_DISPOSITION:
            return "InvalidDisposition";
        case STATUS_INTEGER_DIVIDE_BY_ZERO:
            return "IntegerDivideByZero";
        case STATUS_INTEGER_OVERFLOW:
            return "IntegerOverflow";
        case STATUS_NONCONTINUABLE_EXCEPTION:
            return "NoncontinuableException";
        case STATUS_PRIVILEGED_INSTRUCTION:
            return "PrivilegedInstruction";
        case STATUS_REG_NAT_CONSUMPTION:
            return "RegNatConsumption";
        case STATUS_SINGLE_STEP:
            return "SingleStep";
        case STATUS_STACK_OVERFLOW:
            return "StackOverflow";
    }

    std::ostringstream s;
    s << code;
    return s.str();
}

void ReadEnvironmentVariables(options_class &options, restrictions_class &restrictions) {
    CHAR buffer[1024];
    const struct {
        char *name;
        restriction_kind_t restriction;
    } restriction_bindings[] = {
        {"SP_SECURITY_LEVEL", restriction_security_limit},
        {"SP_TIME_LIMIT", restriction_processor_time_limit},
        {"SP_MEMORY_LIMIT", restriction_memory_limit},
        {"SP_WRITE_LIMIT", restriction_write_limit},
        {"SP_DEADLINE", restriction_user_time_limit},
        {"SP_LOAD_RATIO", restriction_load_ratio},
        {"SP_IDLE_TIME_LIMIT", restriction_idle_time_limit}
    };
    const int restriction_bindings_count =
        sizeof(restriction_bindings)/(sizeof(char*) + sizeof(restriction_kind_t));

    if (GetEnvironmentVariable("SP_JSON", buffer, sizeof(buffer))) {
        options.json = atoi(buffer);
    }

    if (GetEnvironmentVariable("SP_HIDE_REPORT", buffer, sizeof(buffer))) {
        options.hide_report = atoi(buffer);
    }

    if (GetEnvironmentVariable("SP_HIDE_OUTPUT", buffer, sizeof(buffer))) {
        options.hide_output = atoi(buffer);
    }
    for (int i = 0; i < restriction_bindings_count; ++i) {
        if (GetEnvironmentVariable(restriction_bindings[i].name, buffer, sizeof(buffer))) {
            SetRestriction(restrictions, restriction_bindings[i].restriction, buffer);
        }
    }
    if (GetEnvironmentVariable("SP_USER", buffer, sizeof(buffer))) {
        options.login = buffer;
    }

    if (GetEnvironmentVariable("SP_PASSWORD", buffer, sizeof(buffer))) {
        options.password = buffer;
    }

    if (GetEnvironmentVariable("SP_REPORT_FILE", buffer, sizeof(buffer))) {
        options.report_file = buffer;
    }

    if (GetEnvironmentVariable("SP_OUTPUT_FILE", buffer, sizeof(buffer))) {
        options.stdoutput.push_back(buffer);
    }

    if (GetEnvironmentVariable("SP_ERROR_FILE", buffer, sizeof(buffer))) {
        options.stderror.push_back(buffer);
    }

    if (GetEnvironmentVariable("SP_INPUT_FILE", buffer, sizeof(buffer))) {
        options.stdinput.push_back(buffer);
    }
}

#if defined(WANT_STACKWALKER)
std::string get_stacktrace_string() {
    std::stringstream r;
    auto collect = [&](const char* s) {
        r << s;
    };
    class StackWalkerToString : public StackWalker {
        std::function<void(const char*)> f_;
    public:
        StackWalkerToString(std::function<void(const char*)> f) :f_(f) {}
    protected:
        virtual void OnOutput(LPCSTR szText) {
            f_(szText);
        }
    } sw(collect);
    sw.ShowCallstack();
    return r.str();
}
#endif

void make_minidump(EXCEPTION_POINTERS* e) {
#if defined(_MSC_VER)
    auto hDbgHelp = LoadLibraryA("dbghelp");
    if (hDbgHelp == nullptr)
        return;

    auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (pMiniDumpWriteDump == nullptr)
        return;

    char name[MAX_PATH];
    {
        auto nameEnd = name + GetModuleFileNameA(GetModuleHandleA(0), name, MAX_PATH);
        SYSTEMTIME t;
        GetSystemTime(&t);
        wsprintfA(nameEnd - strlen(".exe"),
            "_%4d%02d%02d_%02d%02d%02d.dmp",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }

    auto hFile = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = e;
    exceptionInfo.ClientPointers = FALSE;

    auto dumped = pMiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
        e ? &exceptionInfo : nullptr,
        nullptr,
        nullptr);

    CloseHandle(hFile);

#endif
}

std::string get_win_last_error_string() {
    DWORD error_code = GetLastError();
    char* error_text = nullptr;

    auto format_message = [&](const DWORD lang_id) -> DWORD {
        return FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error_code,
            lang_id,
            (LPSTR)&error_text,
            0, NULL);
    };

    if (format_message(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) == 0) {
        format_message(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    }

    std::stringstream r;
    r << "error code: " << error_code;
    if (error_text != nullptr) {
        r << ": " << error_text;
    }
    LocalFree(error_text);
    return r.str();
}

void platform_exit_failure() {
    ExitProcess(1);
}
