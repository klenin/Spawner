#ifndef _PIPES_H_
#define _PIPES_H_

#include "platform.h"

typedef enum
{
    ACCESS_READ     = 0x1,
    ACCESS_WRITE    = 0x2,
};

class CPipe
{
public:
    CPipe():readPipe(INVALID_HANDLE_VALUE), writePipe(INVALID_HANDLE_VALUE), readPipeDup(INVALID_HANDLE_VALUE)//init with default_handle_value
    {
        Init();
    }
    CPipe(std_handle_t handleType)
    {
        Init();
        if (handleType == STD_OUTPUT || handleType == STD_ERROR)
            SetStdHandle(handleType, writePipe);
        else
            SetStdHandle(handleType, readPipe);
    }
    void Init()
    {
        SECURITY_ATTRIBUTES saAttr;   
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
        saAttr.bInheritHandle = TRUE; 
        saAttr.lpSecurityDescriptor = NULL; 
        if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0))
        {

        }
        //SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
        ;

    }
    HANDLE ReadPipe(){SetHandleInformation(writePipe, HANDLE_FLAG_INHERIT, 0); return readPipe;}
    HANDLE WritePipe(){SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0); return writePipe;}
    HANDLE DuplicateReadPipe()
    {
        if (!DuplicateHandle(GetCurrentProcess(), readPipe,
            GetCurrentProcess(), &readPipeDup, 0, FALSE,
            DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE) || readPipeDup == INVALID_HANDLE_VALUE)
        {

        }

        return readPipeDup;
    }
    void ClosePipe()
    {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
    }
    ~CPipe()
    {
        CloseHandle(readPipe);//replace with safe method
        CloseHandle(writePipe);//replace with safe method
        if (readPipeDup != INVALID_HANDLE_VALUE)
            CloseHandle(readPipeDup);
    }
    bool Write(void *data, size_t size);
    size_t Read(void *data, size_t size);
private:
    HANDLE readPipe, writePipe, readPipeDup;
};

#endif//_PIPES_H_