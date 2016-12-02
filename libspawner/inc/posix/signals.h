#ifndef _SPAWNER_SIGNALS_H_
#define _SPAWNER_SIGNALS_H_

#include <signal.h>

#include "inc/report.h"

enum signal_t
{
    signal_sighup = SIGHUP,
    signal_sigint = SIGINT,
    signal_sigquit = SIGQUIT,
    signal_sigill = SIGILL,
    signal_sigtrap = SIGTRAP,
    signal_abort = SIGABRT,
    signal_sigfpe = SIGFPE,
    signal_sigkill = SIGKILL,
    signal_sigbus = SIGBUS,
    signal_sigsegv = SIGSEGV,
    signal_sigalarm = SIGALRM,
    signal_sigterm = SIGTERM,
    signal_sigsys = SIGSYS,
    signal_sigxcpu = SIGXCPU,
    signal_sigxfsz = SIGXFSZ,
    signal_signal_no = 0, // no signal recieved
    signal_unknown = 0xff,
};

const map_cell event_identifiers[] = {
    {signal_sighup, "SIGHUP", "hangup"},
    {signal_sigint, "SIGINT", "interrupt"},
    {signal_sigquit, "SIGQUIT", "quit"},
    {signal_sigill, "SIGILL", "illegal instruction"},
    {signal_sigtrap, "SIGTRAP", "trace trap"},
    {signal_abort, "SIGABRT", "abort"},
    {signal_sigfpe, "SIGFPE", "floating point exception"},
    {signal_sigkill, "SIGKILL", "kill"},
    {signal_sigbus, "SIGBUS", "bus error"},
    {signal_sigsegv, "SIGSEGV", "segmentation violation"},
    {signal_sigalarm, "SIGALRM", "alarm clock"},
    {signal_sigterm, "SIGTERM", "software termination signal from kill"},
    {signal_sigsys, "SIGSYS", "bad argument to system call"},
    {signal_sigxcpu, "SIGXCPU", "exceeded CPU time limit"},
    {signal_sigxfsz, "SIGXFSZ", "exceeded file size limit"},
    {signal_unknown, nullptr, "unknown signal number, run kill -l to list"},
};

#endif//_SPAWNER_SIGNALS_H_
