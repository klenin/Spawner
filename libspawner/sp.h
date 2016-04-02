#ifndef _SP_H_
#define _SP_H_

#include "inc/runner.h"
#include "inc/securerunner.h"

#if defined(_WIN32) || defined(_WIN64)
#include <inc/pipes.h>
#include "inc/win32/platform.h"
#else
#include "inc/posix/platform.h"
#endif
#include <inc/uconvert.h>

#include "inc/report.h"

#include "inc/error.h"

#include "inc/session.h"
#include <inc/delegate.h>

#endif//_SP_H_
