#ifndef _SP_LINUX_SECCOMP_HPP_
#define _SP_LINUX_SECCOMP_HPP_

/*
 Some macros of this code were taken from this page
 https://outflux.net/teach-seccomp/
*/

/*
seccomp example for x86 (32-bit and 64-bit) with BPF macros

Copyright (c) 2012 The Chromium OS Authors <chromium-os-dev@chromium.org>
Authors:
 Will Drewry <wad@chromium.org>
 Kees Cook <keescook@chromium.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following disclaimer
     in the documentation and/or other materials provided with the
     distribution.
   * Neither the name of Google Inc. nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <errno.h>
#include <stddef.h>
#include <sys/prctl.h>
#ifndef PR_SET_NO_NEW_PRIVS
# define PR_SET_NO_NEW_PRIVS 38
#endif

#include <linux/audit.h>
#include <linux/unistd.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#define syscall_nr (offsetof(struct seccomp_data, nr))
#define arch_nr (offsetof(struct seccomp_data, arch))


#if defined(__i386__)
# define REG_SYSCALL    REG_EAX
# define ARCH_NR        AUDIT_ARCH_I386
#elif defined(__x86_64__)
# define REG_SYSCALL    REG_RAX
# define ARCH_NR        AUDIT_ARCH_X86_64
#endif

#define VALIDATE_ARCHITECTURE \
        BPF_STMT(BPF_LD+BPF_W+BPF_ABS, arch_nr), \
        BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ARCH_NR, 1, 0), \
        BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL)

#define EXAMINE_SYSCALL \
        BPF_STMT(BPF_LD+BPF_W+BPF_ABS, syscall_nr)

#define ALLOW_SYSCALL(name) \
        BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, __NR_##name, 0, 1), \
        BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

#define KILL_PROCESS \
        BPF_STMT(BPF_RET+BPF_K, RESTRICTION_TYPE)

#define RESTRICTION_TYPE SECCOMP_RET_KILL // silently kill child
//#define RESTRICTION_TYPE SECCOMP_RET_TRAP // send SIGSYS

#define SECCOMP_MODE_FILTER    2 /* uses user-supplied filter. */
#define SECCOMP_RET_KILL       0x00000000U /* kill the task immediately */
#define SECCOMP_RET_TRAP       0x00030000U /* disallow and force a SIGSYS */
#define SECCOMP_RET_ALLOW      0x7fff0000U /* allow */

// syscalls filters to allow execve()
static struct sock_filter filter[] = {
    VALIDATE_ARCHITECTURE,
    EXAMINE_SYSCALL,
    ALLOW_SYSCALL(rt_sigreturn),
    ALLOW_SYSCALL(exit_group),
    ALLOW_SYSCALL(exit),
    ALLOW_SYSCALL(read),
    ALLOW_SYSCALL(write),
    ALLOW_SYSCALL(rt_sigprocmask),
    ALLOW_SYSCALL(rt_sigaction),
    ALLOW_SYSCALL(nanosleep),
    ALLOW_SYSCALL(brk),
    ALLOW_SYSCALL(execve),
    ALLOW_SYSCALL(close),
    ALLOW_SYSCALL(open),
    ALLOW_SYSCALL(access),
    ALLOW_SYSCALL(fstat),
    ALLOW_SYSCALL(mmap),
#if defined(__x86_64__)
    ALLOW_SYSCALL(arch_prctl),
#endif
    ALLOW_SYSCALL(munmap),
    ALLOW_SYSCALL(mprotect),
    KILL_PROCESS,
};
static struct sock_fprog prog = {
    .len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
    .filter = filter,    
};
 
int seccomp_probe_filter();
int seccomp_setup_filter();

#endif // _SP_LINUX_SECCOMP_HPP_
