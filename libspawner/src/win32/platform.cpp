#include "platform.h"

void CloseHandleSafe(HANDLE &handle)
{
    if (handle == INVALID_HANDLE_VALUE)
        return;
    CloseHandle(handle);
    handle = INVALID_HANDLE_VALUE;
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
