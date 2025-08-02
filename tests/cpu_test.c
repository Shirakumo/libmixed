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

#include "../src/ffts_cpu.h"

#include <stdio.h>

int main()
{
    int cpu_flags, extra_flags;

    cpu_flags = ffts_cpu_detect(&extra_flags);
    if (!cpu_flags)
        return 1;

    printf("SSE    : %s\n", (cpu_flags & FFTS_CPU_X86_SSE) ? "yes" : "no");
    printf("SSE2   : %s\n", (cpu_flags & FFTS_CPU_X86_SSE2) ? "yes" : "no");
    printf("SSE3   : %s\n", (cpu_flags & FFTS_CPU_X86_SSE3) ? "yes" : "no");
    printf("SSSE3  : %s\n", (cpu_flags & FFTS_CPU_X86_SSSE3) ? "yes" : "no");
    printf("SSE4.1 : %s\n", (cpu_flags & FFTS_CPU_X86_SSE4_1) ? "yes" : "no");
    printf("SSE4.2 : %s\n", (cpu_flags & FFTS_CPU_X86_SSE4_2) ? "yes" : "no");
    printf("AVX    : %s\n", (cpu_flags & FFTS_CPU_X86_AVX) ? "yes" : "no");
    printf("AVX2   : %s\n", (cpu_flags & FFTS_CPU_X86_AVX2) ? "yes" : "no");
    printf("AVX512 : %s\n", (cpu_flags & FFTS_CPU_X86_AVX512) ? "yes" : "no");
    return 0;
}
