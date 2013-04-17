#include "platform.h"

void CloseHandleSafe(HANDLE &handle)
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
