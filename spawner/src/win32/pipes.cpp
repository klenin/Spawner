#include "pipes.h"

bool CPipe::Write(void *data, size_t size)
{
    BOOL bSuccess;
    DWORD dwWritten;
    bSuccess = WriteFile(writePipe, data, size, &dwWritten, NULL);
    if (!bSuccess || dwWritten != size)
    {
        return false;
    }
    return true;
}

size_t CPipe::Read(void *data, size_t size)
{
    BOOL bSuccess;
    DWORD dwRead;
    if (readPipeDup != INVALID_HANDLE_VALUE)
        bSuccess = ReadFile(readPipeDup, data, size, &dwRead, NULL);
    else
        bSuccess = ReadFile(readPipe, data, size, &dwRead, NULL);
    if (!bSuccess)
    {
        //error
    }
    return dwRead;
}
