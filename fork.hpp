#ifndef FORK_HPP
#define FORK_HPP

#include <sys/syscall.h>

inline 
long clone(unsigned long flags, void *child_stack, void *ptid, void *ctid, struct pt_regs *regs)
    {
    return syscall( SYS_clone, flags, child_stack, ptid, ctid, regs );
    }

inline long fork(unsigned long flags)
    {
    return clone( flags, nullptr, nullptr, nullptr, nullptr );
    }

#endif

