#ifndef _SPAWNER_SIGNALS_H_
#define _SPAWNER_SIGNALS_H_

#include "signal.h"

enum signal_t
{
    signal_sighup = SIGHUP,
    signal_sigint = SIGINT,
    signal_sigquit = SIGQUIT,
    signal_sigill = SIGILL,
    signal_abort = SIGABRT,
    signal_sigfpe = SIGFPE,
    signal_sigkill = SIGKILL,
    signal_sigbus = SIGBUS,
    signal_sigsegv = SIGSEGV,
    signal_sigterm = SIGTERM,
    signal_sigsys = SIGSYS,
    signal_sigxcpu = SIGXCPU,
    signal_sigxfsz = SIGXFSZ,
    signal_signal_no = 0, // no signal recieved
    signal_unregistered = 0xff,
};

#endif//_SPAWNER_SIGNALS_H_
