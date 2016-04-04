#include <fstream>
#include <string>
#include "platform.h"

#include <WinBase.h>
// windows8 requires Processenv.h for GetEnvironmentVariableA()

//#include <Windows.h>
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
