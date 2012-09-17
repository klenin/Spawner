#include "platform.h"

void CloseHandleSafe(HANDLE &handle)
{
    if (handle == INVALID_HANDLE_VALUE)
        return;
    CloseHandle(handle);
    handle = INVALID_HANDLE_VALUE;
}