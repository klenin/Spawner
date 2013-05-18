#ifndef _SPAWNER_PLATFORM_H_
#define _SPAWNER_PLATFORM_H_

/************************************************************************/
/* GLOBAL TODO                                                          */
/* * Replace all string with std::string                                */
/* * Make report.cpp kinda cross-platform                               */
/************************************************************************/

#include <stdlib.h>
#ifdef _WIN32

//#ifndef _MSC_VER
#if _WIN32_WINNT < 0x0501
#define _WIN32_WINNT 0x0501
#endif
//#endif//_MSC_VER
#include <windows.h>
#include <tlhelp32.h>
//#include <winioctl.h>

typedef HANDLE thread_t;

#define thread_return_t DWORD WINAPI
#define thread_param_t LPVOID

#define COMPLETION_KEY 1

#define JOB_OBJECT_MSG_PROCESS_WRITE_LIMIT 11
#define JOB_OBJECT_MSG_PROCESS_USER_TIME_LIMIT 12
#define JOB_OBJECT_MSG_PROCESS_LOAD_RATIO_LIMIT 13
#define SECOND_COEFF 10000

typedef PROCESS_INFORMATION process_info_t;
typedef HANDLE pipe_t;
typedef STARTUPINFO startupinfo_t;
typedef DWORD process_id;

const DWORD PROCESS_CREATION_FLAGS = (CREATE_SUSPENDED | /*CREATE_PRESERVE_CODE_AUTHZ_LEVEL | */CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB);

void CloseHandleSafe(HANDLE &handle);


const unsigned long exit_code_ok = 0;
const unsigned long exit_code_still_active = STILL_ACTIVE;
const unsigned long exit_code_exception_int_divide_by_zero = EXCEPTION_INT_DIVIDE_BY_ZERO;

const unsigned long infinite = INFINITE;

const handle_t handle_default_value = INVALID_HANDLE_VALUE;

#else

#endif//_WIN32

wchar_t *a2w(const char *str);

#endif//_SPAWNER_PLATFORM_H_
