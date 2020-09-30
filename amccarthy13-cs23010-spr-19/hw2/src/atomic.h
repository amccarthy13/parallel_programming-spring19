//
// Created by amccarthy13 on 4/24/19.
//

#ifndef CS_23010_ATOMIC_H
#define CS_23010_ATOMIC_H

#ifdef __x86_64__
typedef __uint128_t DWORD;
#else
typedef __uint64_t DWORD:
#endif

#ifdef GCC_ATOMIC_BUILTINS
#define CAS __sync_bool_compare_and_swap
#define DWCAS __sync_bool_compare_and_swap
#else

#ifdef __x86_64__
#define SHIFT 	  64
#define XADDx	  "xaddq"
#define CMPXCHGxB "cmpxchg16b"
#else
#define SHIFT 	  32
#define XADDx	  "xaddl"
#define CMPXCHGxB "cmpxchg8b"
#endif

static inline
char CAS(volatile unsigned long *mem, unsigned long old, unsigned long new) {
    unsigned long r;
    asm volatile("lock cmpxchgl %k2,%1"
                : "=a" (r), "+m" (*mem)
                : "r" (new), "0" (old)
                : "memory");
    return r == old ? 1 : 0;
}

static inline
char DWCAS(volatile DWORD *mem, DWORD old, DWORD new)
{
    unsigned long old_h = old >> SHIFT, old_l = old;
    unsigned long new_h = new >> SHIFT, new_l = new;

    char r = 0;
    asm volatile("lock; " CMPXCHGxB " (%6);"
                 "setz %7; "
                : "=a" (old_l),
                "=d" (old_h)
                : "0" (old_l),
                "1" (old_h),
                "b" (new_l),
                "c" (new_h),
                "r" (mem),
                "m" (r)
                : "cc", "memory");
    return r;
}

#endif

#endif
