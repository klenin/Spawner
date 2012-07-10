#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifdef _WIN32

#include <Windows.h>

typedef HANDLE thread_t;

#define thread_return_t DWORD WINAPI
#define thread_param_t LPVOID

typedef PROCESS_INFORMATION process_info_t;

typedef enum
{
    STD_INPUT   = STD_INPUT_HANDLE,
    STD_OUTPUT  = STD_OUTPUT_HANDLE,
    STD_ERROR   = STD_ERROR_HANDLE,
}std_handle_t;

#else

#endif//_WIN32

#endif//_PLATFORM_H_