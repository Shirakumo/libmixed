/*

This file is part of FFTS -- The Fastest Fourier Transform in the South

Copyright (c) 2018, Jukka Ojanen <jukka.ojanen@kolumbus.fi>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the organization nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANTHONY M. BLAKE BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ffts_cpu.h"

#if defined(FFTS_BUILDING_CPU_TEST)
#include <stdio.h>
#endif

#if defined(_WIN32)
#include <intrin.h>
#include <windows.h>
#endif

/* TODO: add detection/declaration of these to CMake phase */
#if !defined(FFTS_CPU_X64)
#if defined(_M_AMD64) || defined(__amd64) || defined(__amd64__) || defined(_M_X64) || defined(__x86_64) || defined(__x86_64__)
/* 64 bit x86 detected */
#define FFTS_CPU_X64
#endif
#endif

#if !defined(FFTS_CPU_X64) && !defined(FFTS_CPU_X86)
#if defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__X86__) || defined(_X86_)
/* 32 bit x86 detected */
#define FFTS_CPU_X86
#endif
#endif

/* check if build is 32 bit or 64 bit x86 */
#if defined(FFTS_CPU_X64) || defined(FFTS_CPU_X86)

/* Build and tested on
CentOS 6.8 2.6.32-642.11.1.el6.x86_64 - gcc version 4.4.7 20120313
Mac OSX 10.9 - Apple Clang 6.0
Ubuntu 14.04 LTS 4.2.0-42 x86_64 - gcc version 4.8.4
Windows XP SP3 - Visual Studio 2005 SP1 x86/x64
Windows Vista SP2 - Visual Studio 2010 SP1 x86/x64
Windows 7 Ultimate SP1 - Visual Studio 2015 x86/x64
Windows 7 Ultimate SP1 - gcc version 4.9.2 (i686-posix-dwarf-rev1)
Windows 7 Ultimate SP1 - gcc version 4.9.2 (x86_64-posix-seh-rev3)
Windows 10 Pro - Visual Studio 2017 x86/x64
*/

/* Visual Studio 2010 SP1 or newer have _xgetbv intrinsic */
#if (defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 160040219)
#define FFTS_HAVE_XGETBV
#endif

#ifndef BIT
#define BIT(n) (1u << n)
#endif

/* bit masks */
#define FFTS_CPU_X86_SSE_BITS    (BIT(0) | BIT(15) | BIT(23) | BIT(24) | BIT(25))
#define FFTS_CPU_X86_SSE2_BITS   (BIT(26))
#define FFTS_CPU_X86_SSE3_BITS   (BIT(0))
#define FFTS_CPU_X86_SSSE3_BITS  (BIT(9))
#define FFTS_CPU_X86_SSE4_1_BITS (BIT(19))
#define FFTS_CPU_X86_SSE4_2_BITS (BIT(20) | BIT(23))
#define FFTS_CPU_X86_AVX_BITS    (BIT(26) | BIT(27) | BIT(28))
#define FFTS_CPU_X86_XCR0_BITS   (
#define FFTS_CPU_X86_AVX2_BITS   (BIT(5))
#define FFTS_CPU_X86_AVX512_BITS (BIT(16))

/* Visual Studio 2008 or older */
#if defined(FFTS_CPU_X64) && defined(_MSC_VER) && _MSC_VER <= 1500
#pragma optimize("", off)
static void __fastcall ffts_cpuidex(int subleaf, int regs[4], int leaf)
{
    /* x64 uses a four register fast-call calling convention by default and
       arguments are passed in registers RCX, RDX, R8, and R9. By disabling
       optimization and passing subleaf as first argument we get __cpuidex
    */
    (void) subleaf;
    __cpuid(regs, leaf);
}
#pragma optimize("", on)
#endif

static FFTS_INLINE void ffts_cpuid(int regs[4], int leaf, int subleaf)
{
#if defined(_MSC_VER)
#if defined(FFTS_CPU_X64)
    /* Visual Studio 2010 or newer */
#if _MSC_VER > 1500
    __cpuidex(regs, leaf, subleaf);
#else
    ffts_cpuidex(subleaf, regs, leaf);
#endif
#else
    __asm {
        mov eax, leaf
        mov ecx, subleaf
        mov esi, regs
        cpuid
        mov [esi + 0x0], eax
        mov [esi + 0x4], ebx
        mov [esi + 0x8], ecx
        mov [esi + 0xc], edx
    }
#endif
#elif defined(__GNUC__) && __GNUC__
#if defined(FFTS_CPU_X64)
    __asm__ __volatile__(
        "cpuid\n\t"
        : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(leaf), "c"(subleaf));
#elif defined(__PIC__)
    __asm__ __volatile__(
        "xchgl %%ebx, %1\n\t"
        "cpuid          \n\t"
        "xchgl %%ebx, %1\n\t"
        : "=a"(regs[0]), "=r"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(leaf), "c"(subleaf));
#else
    __asm__ __volatile__(
        "cpuid\n\t"
        : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(leaf), "c"(subleaf));
#endif
#else
    /* unknown compiler for x86 */
    regs[0] = regs[1] = regs[2] = regs[3] = 0;
#endif
}

/* at least Visual Studio 2010 generates invalidate optimized _xgetbv */
#if defined(FFTS_HAVE_XGETBV)
#pragma optimize("", off)
#endif
static FFTS_INLINE unsigned int ffts_get_xcr0(void)
{
#if defined(FFTS_HAVE_XGETBV)
    return (unsigned int) _xgetbv(0);
#elif defined(_MSC_VER)
#if defined(FFTS_CPU_X64)
    /* emulate xgetbv(0) on Windows 7 SP1 or newer */
    typedef DWORD64 (WINAPI *PGETENABLEDXSTATEFEATURES)(VOID);
    PGETENABLEDXSTATEFEATURES pfnGetEnabledXStateFeatures = 
        (PGETENABLEDXSTATEFEATURES) GetProcAddress(
        GetModuleHandle(TEXT("kernel32.dll")), "GetEnabledXStateFeatures");
    return pfnGetEnabledXStateFeatures ? (unsigned int) pfnGetEnabledXStateFeatures() : 0;
#else
    /* note that we have to touch edx register to tell compiler it's used by emited xgetbv */
    unsigned __int32 hi, lo;
    __asm {
        xor ecx, ecx
        _emit 0x0f
        _emit 0x01
        _emit 0xd0
        mov lo, eax
        mov hi, edx
    }
    return (unsigned int) lo;
#endif
#elif defined(__GNUC__) && __GNUC__
    unsigned int lo;
    __asm__ __volatile__(".byte 0x0f, 0x01, 0xd0\n"
        : "=a"(lo)
        : "c"(0)
        : "edx");
    return lo;
#else
    /* unknown x86 compiler */
    return 0;
#endif
}
#if defined(FFTS_HAVE_XGETBV)
#pragma optimize("", on)
#endif

int
ffts_cpu_detect(int *extra_flags)
{
    static int cpu_flags = -1;
    static int cpu_extra_flags = -1;
    int max_basic_func;
    int regs[4];
    unsigned int xcr0;

    if (cpu_flags >= 0) {
        goto exit;
    }

    /* initialize */
    cpu_flags = cpu_extra_flags = 0;

#if defined(FFTS_BUILDING_CPU_TEST)
    printf("cpuid check: ");
#endif
#if defined(FFTS_CPU_X64)
    /* cpuid is always supported on x64 */
#if defined(FFTS_BUILDING_CPU_TEST)
    printf("skipped\n");
#endif
#else
#if defined(_MSC_VER)
    _asm {
        pushfd
        pop eax
        mov ebx,eax
        xor eax,200000h
        push eax
        popfd
        pushfd
        pop eax
        push ebx
        popfd
        mov regs[0 * TYPE regs],eax
        mov regs[1 * TYPE regs],ebx
    }
#else
    __asm__ (
        "pushfl\n\t"
        "pop %0\n\t"
        "movl %0,%1\n\t"
        "xorl $0x200000,%0\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        "pushl %1\n\t"
        "popfl\n\t"
        : "=r" (regs[0]), "=r" (regs[1])
    );
#endif
    /* check CPUID bit (bit 21) in EFLAGS register can be toggled */
    if (((regs[0] ^ regs[1]) & 0x200000) == 0) {
#if defined(FFTS_BUILDING_CPU_TEST)
        printf("not supported\n");
#endif
        goto exit;
    }
#if defined(FFTS_BUILDING_CPU_TEST)
        printf("supported\n");
#endif
#endif

    /* get the number of basic functions */
    ffts_cpuid(regs, 0, 0);
    max_basic_func = regs[0];
#if defined(FFTS_BUILDING_CPU_TEST)
    printf("cpuid eax=0, ecx=0: %d\n", max_basic_func);
#endif
    if (max_basic_func == 0)
        goto exit;

    /* get feature flags */
    ffts_cpuid(regs, 1, 0);

#if defined(FFTS_BUILDING_CPU_TEST)
    printf("cpuid eax=1, ecx=0: eax=%08x ebx=%08x ecx=%08x edx=%08x\n", regs[0], regs[1], regs[2], regs[3]);
#endif

#if defined(FFTS_CPU_X64)
    /* minimum for any x64 */
    cpu_flags = FFTS_CPU_X86_SSE | FFTS_CPU_X86_SSE2;
#else
    /* test if SSE is supported */
    if ((regs[3] & FFTS_CPU_X86_SSE_BITS) != FFTS_CPU_X86_SSE_BITS)
        goto exit;
    cpu_flags = FFTS_CPU_X86_SSE;

    /* test if SSE2 is supported */
    if (!(regs[3] & FFTS_CPU_X86_SSE2_BITS))
        goto exit;
    cpu_flags |= FFTS_CPU_X86_SSE2;
#endif

    /* test if SSE3 is supported */
    if (!(regs[2] & FFTS_CPU_X86_SSE3_BITS))
        goto exit;
    cpu_flags |= FFTS_CPU_X86_SSE3;

    /* test if SSSE3 is supported */
    if (!(regs[2] & FFTS_CPU_X86_SSSE3_BITS))
        goto exit;
    cpu_flags |= FFTS_CPU_X86_SSSE3;

    /* test if SSE4.1 is supported */
    if (!(regs[2] & FFTS_CPU_X86_SSE4_1_BITS))
        goto exit;
    cpu_flags |= FFTS_CPU_X86_SSE4_1;

    /* test if SSE4.2 is supported */
    if ((regs[2] & FFTS_CPU_X86_SSE4_2_BITS) != FFTS_CPU_X86_SSE4_2_BITS)
        goto exit;
    cpu_flags |= FFTS_CPU_X86_SSE4_2;

    /* test if AVX is supported */
    if ((regs[2] & FFTS_CPU_X86_AVX_BITS) != FFTS_CPU_X86_AVX_BITS)
        goto exit;

    /* test if legaxy x87, 128-bit SSE and 256-bit AVX states are enabled in XCR0 */
    xcr0 = ffts_get_xcr0();
#if defined(FFTS_BUILDING_CPU_TEST)
    printf("xcr0: %u\n", xcr0);
#endif
    if ((xcr0 & 0x6) != 0x6)
        goto exit;

    cpu_flags |= FFTS_CPU_X86_AVX;

    /* check that cpuid extended features exist */
    if (max_basic_func < 7)
        goto exit;

    /* get extended features */
    ffts_cpuid(regs, 7, 0);

#if defined(FFTS_BUILDING_CPU_TEST)
    printf("cpuid eax=7, ecx=0: eax=%08x ebx=%08x ecx=%08x edx=%08x\n", regs[0], regs[1], regs[2], regs[3]);
#endif

    /* test if AVX2 is supported */
    if ((regs[1] & FFTS_CPU_X86_AVX2_BITS) != FFTS_CPU_X86_AVX2_BITS)
        goto exit;
    cpu_flags |= FFTS_CPU_X86_AVX2;

    /* test if AVX512 is supported */
    if ((regs[1] & FFTS_CPU_X86_AVX512_BITS) != FFTS_CPU_X86_AVX512_BITS)
        goto exit;
    cpu_flags |= FFTS_CPU_X86_AVX512;

exit:
    if (extra_flags) {
        *extra_flags = cpu_extra_flags;
    }
    return cpu_flags;
}
#else 
int
ffts_cpu_detect(int *extra_flags)
{
    /* not implemented */
#if defined(FFTS_BUILDING_CPU_TEST)
    printf("CPU detection not implemented!!\n");
#endif
    return 0;
}
#endif