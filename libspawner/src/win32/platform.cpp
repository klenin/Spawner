#include "platform.h"

#include <sstream>
#include <fstream>
#include <string>

#include <cstdlib>

#include <Windows.h>
#ifdef _MSC_VER
#include <DbgHelp.h>
#endif

#include "stack_walker.h"
#include "inc/uconvert.h"
#include "inc/error.h"
#include "compatibility.h"

void CloseHandleSafe_debug(HANDLE &handle, char *file, unsigned int line)
{
    try {
        if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
            return;
        if (!CloseHandle(handle))
            PANIC(get_win_last_error_string());
    } catch (...) {
        std::ofstream log_file("C:\\CATS\\cats-judge\\log.log", std::ofstream::app);
        log_file << file << ":" << line << " " << handle << std::endl;
    }
    handle = INVALID_HANDLE_VALUE;
}

void CloseHandleSafe_real(HANDLE &handle)
{
    if (handle == INVALID_HANDLE_VALUE || handle == nullptr)
        return;
    if (!CloseHandle(handle))
        PANIC(get_win_last_error_string());
    handle = INVALID_HANDLE_VALUE;
}

int get_spawner_pid()
{
    return (int) GetCurrentProcessId();
}

void push_shm_report(const char *shm_name, const std::string &report)
{
    HANDLE hOut = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, shm_name);
    LPCSTR pRep = (LPTSTR)MapViewOfFile(
        hOut,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        options_class::SHARED_MEMORY_BUF_SIZE
    );

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
        case STATUS_PENDING:
            return "Still running";
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

std::string get_win_last_error_string(PDWORD_PTR args) {
    DWORD error_code = GetLastError();
    char* error_text = nullptr;

    auto format_message = [&](const DWORD lang_id) -> DWORD {
        return FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            (args ? FORMAT_MESSAGE_ARGUMENT_ARRAY : FORMAT_MESSAGE_IGNORE_INSERTS),
            nullptr,
            error_code,
            lang_id,
            (LPSTR)&error_text,
            0, (va_list*)args);
    };

    if (format_message(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) == 0) {
        format_message(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    }

    std::stringstream r;
    r << "error " << error_code;
    if (error_text) {
        r << ": " << error_text;
    }
    LocalFree(error_text);
    return r.str();
}

void platform_exit_failure() {
    ExitProcess(1);
}

