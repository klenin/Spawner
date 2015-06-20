#pragma once

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
#define JOB_OBJECT_MSG_PROCESS_COUNT_LIMIT 14
#define JOB_OBJECT_MSG_PROCESS_CONTROLLER_STOP 15
#define SECOND_COEFF 10000

#ifndef STATUS_FLOAT_MULTIPLE_FAULTS
#define STATUS_FLOAT_MULTIPLE_FAULTS     ((DWORD   )0xC00002B4L)
#endif//STATUS_FLOAT_MULTIPLE_FAULTS

#ifndef STATUS_FLOAT_MULTIPLE_TRAPS
#define STATUS_FLOAT_MULTIPLE_TRAPS      ((DWORD   )0xC00002B5L)
#endif//STATUS_FLOAT_MULTIPLE_TRAPS

#ifndef STATUS_REG_NAT_CONSUMPTION
#define STATUS_REG_NAT_CONSUMPTION       ((DWORD   )0xC00002C9L)
#endif//STATUS_REG_NAT_CONSUMPTION

#ifndef JOB_OBJECT_LIMIT_BREAKAWAY_OK
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK               0x00000800
#endif//JOB_OBJECT_LIMIT_BREAKAWAY_OK

#ifndef JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        0x00001000
#endif//JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK

#ifndef JOB_OBJECT_ASSIGN_PROCESS
#define JOB_OBJECT_ASSIGN_PROCESS           (0x0001)
#endif//JOB_OBJECT_ASSIGN_PROCESS

#ifndef OpenJobObject
#define OPEN_JOB_OBJECT_DYNAMIC_LOAD

void load_open_job_object();

typedef HANDLE (WINAPI *OPEN_JOB_OBJECT)(DWORD, BOOL, LPCSTR);

static OPEN_JOB_OBJECT OpenJobObjectA;

#ifdef UNICODE
#define OpenJobObject  OpenJobObjectW
#else
#define OpenJobObject  OpenJobObjectA
#endif
#endif//OpenJobObject

typedef PROCESS_INFORMATION process_info_t;
typedef HANDLE pipe_t;
typedef STARTUPINFO startupinfo_t;
typedef DWORD process_id;

const DWORD PROCESS_CREATION_FLAGS = (CREATE_SUSPENDED | /*CREATE_PRESERVE_CODE_AUTHZ_LEVEL | */CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB);

#define CloseHandleSafe(handle) (CloseHandleSafe_debug(handle, __FILE__, __LINE__))
void CloseHandleSafe_debug(HANDLE &handle, char *file, unsigned int line);
void CloseHandleSafe_real(HANDLE &handle);

const unsigned long exit_code_ok = 0;
const unsigned long exit_code_still_active = STILL_ACTIVE;
const unsigned long exit_code_exception_int_divide_by_zero = EXCEPTION_INT_DIVIDE_BY_ZERO;

const unsigned long infinite = INFINITE;

const handle_t handle_default_value = INVALID_HANDLE_VALUE;

#else

#endif//_WIN32

BOOL WINAPI CancelSynchronousIo_wrapper(HANDLE);

void platform_init();

wchar_t *a2w(const char *str);
char *w2a(const wchar_t *str);

#ifndef uint
typedef unsigned int uint_32;
typedef uint_32 uint;
#endif//uint
