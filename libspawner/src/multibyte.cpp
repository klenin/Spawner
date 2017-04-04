#include "multibyte.h"

wchar_t *a2w(const char *str)
{
    if (!str)
        return nullptr;
    size_t len = strlen(str);
    wchar_t *wstr = new wchar_t[len + 1];
    wstr[len] = 0;
    mbstowcs(wstr, str, len);
    return wstr;
}

char *w2a(const wchar_t *str) {
    if (!str)
        return nullptr;
    size_t len = wcslen(str);
    char *cstr = new char[len + 1];
    cstr[len] = 0;
    wcstombs(cstr, str, len);
    return cstr;
}
