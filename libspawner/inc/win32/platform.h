#ifndef _WIN_PLATFORM_H_
#define _WIN_PLATFORM_H_

/************************************************************************/
/* GLOBAL TODO                                                          */
/* * Replace all string with std::string                                */
/* * Make report.cpp kinda cross-platform                               */
/************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "inc/restrictions.h"
#include "inc/options.h"
#include "platform_report.h"

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
#endif

#ifndef STATUS_FLOAT_MULTIPLE_TRAPS
#define STATUS_FLOAT_MULTIPLE_TRAPS      ((DWORD   )0xC00002B5L)
#endif

#ifndef STATUS_REG_NAT_CONSUMPTION
#define STATUS_REG_NAT_CONSUMPTION       ((DWORD   )0xC00002C9L)
#endif

#ifndef JOB_OBJECT_LIMIT_BREAKAWAY_OK
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK               0x00000800
#endif

#ifndef JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        0x00001000
#endif

#ifndef JOB_OBJECT_ASSIGN_PROCESS
#define JOB_OBJECT_ASSIGN_PROCESS           (0x0001)
#endif

typedef PROCESS_INFORMATION process_info_t;
typedef HANDLE pipe_handle;
typedef STARTUPINFO startupinfo_t;
typedef DWORD process_id;

const DWORD PROCESS_CREATION_FLAGS = (CREATE_SUSPENDED | /*CREATE_PRESERVE_CODE_AUTHZ_LEVEL | */CREATE_SEPARATE_WOW_VDM | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB);

#define CloseHandleSafe(handle) (CloseHandleSafe_real(handle))
void CloseHandleSafe_debug(HANDLE &handle, const char *file, unsigned int line);
void CloseHandleSafe_real(HANDLE &handle);

const unsigned long infinite = INFINITE;

#ifndef uint
typedef unsigned int uint_32;
typedef uint_32 uint;
#endif//uint

int get_spawner_pid();

void push_shm_report(const char *, const std::string &);
void pull_shm_report(const char *, std::string &);

size_t get_env_var(const char *, char *, size_t);

#if defined(WANT_STACKWALKER)
std::string get_stacktrace_string();
#endif

void make_minidump(EXCEPTION_POINTERS* e);
std::string get_win_last_error_string(PDWORD_PTR args = nullptr);

void platform_exit_failure();

std::string ExtractExitStatus(const report_class &);

#endif //_WIN_PLATFORM_H_
