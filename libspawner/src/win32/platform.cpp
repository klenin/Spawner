#include "platform.h"
#include <fstream>
#include <string>
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

wchar_t *a2w(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    wchar_t *wstr = new wchar_t[len + 1];
    wstr[len] = 0;
    mbstowcs(wstr, str, len);
    return wstr;
}

char *w2a(const wchar_t *str) {
    if (!str)
        return NULL;
    size_t len = wcslen(str);
    char *cstr = new char[len + 1];
    cstr[len] = 0;
    wcstombs(cstr, str, len);
    return cstr;

}
