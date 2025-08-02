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

#include "../src/ffts_internal.h"
#include "../src/ffts_trig.h"

#ifdef HAVE_MPFR_H
#include <mpfr.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#include <stdio.h>

/* 32 bit FNV-1a initial hash */
#define FNV1A_32_INIT ((uint32_t) 0x811c9dc5)

/* 32 bit magic FNV-1a prime */
#define FNV_32_PRIME ((uint32_t) 0x01000193)

#define MAX_LEN 29

/* perform a 32 bit Fowler/Noll/Vo FNV-1a hash on a buffer */
static uint32_t
fnv_32a_buf(const void *buffer, size_t buffer_len, uint32_t hash_value)
{
    const unsigned char *bp = (const unsigned char*) buffer;
    const unsigned char *be = bp + buffer_len;

    while (bp < be) {
        hash_value ^= (uint32_t) *bp++;
        hash_value *= FNV_32_PRIME;
    }

    return hash_value;
}

#ifdef HAVE_MPFR_H
union float_bits {
    int32_t i;
    float f;
};

union double_bits {
    int64_t i;
    double d;
};

static uint64_t distance_between_doubles(double lhs, double rhs)
{
    union double_bits l, r;
    l.d = lhs;
    r.d = rhs;
    return (l.i >= r.i) ? (l.i - r.i) : (r.i - l.i);
}

static uint32_t distance_between_floats(float lhs, float rhs)
{
    union float_bits l, r;
    l.f = lhs;
    r.f = rhs;
    return (l.i >= r.i) ? (l.i - r.i) : (r.i - l.i);
}

static int test_float(int i)
{
    mpfr_t pi, *cop, *rop, *sop;
    int j, len, threads;
    ffts_cpx_32f *table = NULL;
    volatile int mismatch;
    uint32_t hash;

#ifdef _OPENMP
    threads = omp_get_max_threads();
#else
    threads = 1;
#endif
    fprintf(stdout, "Using %d threads\r\n", threads);

    cop = (mpfr_t*) malloc(threads * sizeof(*rop));
    rop = (mpfr_t*) malloc(threads * sizeof(*rop));
    sop = (mpfr_t*) malloc(threads * sizeof(*rop));	
    if (!cop || !rop || !sop) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    mpfr_set_default_prec(256);
    mpfr_init(pi);
    mpfr_const_pi(pi, MPFR_RNDN);
    for (j = 0; j < threads; j++) {
        mpfr_init(cop[j]);
        mpfr_init(rop[j]);
        mpfr_init(sop[j]);
    }

    fprintf(stdout, "Verify single precision trig table using MPFR:\r\n");

    mismatch = 0;
    for (len = (1 << i); len > 0; i--, len >>= 1) {
        fprintf(stdout, "%9d: ", len);
        fflush(stdout);

        if (!table) {
            size_t buffer_len = len * sizeof(*table);
            if (buffer_len < (size_t) len) {
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
            table = (ffts_cpx_32f*) ffts_aligned_malloc(buffer_len);
            if (!table) { 
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
        }

        ffts_generate_cosine_sine_pow2_32f(table, len);
        hash = fnv_32a_buf(table, len * sizeof(*table), FNV1A_32_INIT);

#ifdef _OPENMP
#pragma omp parallel for shared(mismatch)
#endif
        for (j = 1; j <= len/2; j++) {
            float c, s;
#ifdef _OPENMP
            int tid = omp_get_thread_num();
#else
            int tid = 0;
#endif
            if (mismatch)
                continue;
            mpfr_mul_si(rop[tid], pi, -j, MPFR_RNDN);
            mpfr_div_ui(rop[tid], rop[tid], 2 * len, MPFR_RNDN);
            mpfr_sin_cos(sop[tid], cop[tid], rop[tid], MPFR_RNDN);
            c = mpfr_get_flt(cop[tid], MPFR_RNDN);
            s = mpfr_get_flt(sop[tid], MPFR_RNDN);
            if (distance_between_floats(c, table[j][0]) || distance_between_floats(s, table[j][1])) {
                mismatch = 1;
            }
        }

        fprintf(stdout, "%s (checksum: %u)\r\n", mismatch ? "incorrect" : "ok", hash);
        if (mismatch)
            break;
    }

    free(cop);
    free(rop);
    free(sop);
    return mismatch;
}

static int test_double(int i)
{
    mpfr_t pi, *cop, *rop, *sop;
    int j, len, threads;
    ffts_cpx_64f *table = NULL;
    volatile int mismatch;
    uint32_t hash;

#ifdef _OPENMP
    threads = omp_get_max_threads();
#else
    threads = 1;
#endif
    fprintf(stdout, "Using %d threads\r\n", threads);

    cop = (mpfr_t*) malloc(threads * sizeof(*rop));
    rop = (mpfr_t*) malloc(threads * sizeof(*rop));
    sop = (mpfr_t*) malloc(threads * sizeof(*rop));
    if (!cop || !rop || !sop) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    mpfr_set_default_prec(256);
    mpfr_init(pi);
    mpfr_const_pi(pi, MPFR_RNDN);
    for (j = 0; j < threads; j++) {
        mpfr_init(cop[j]);
        mpfr_init(rop[j]);
        mpfr_init(sop[j]);
    }

    fprintf(stdout, "Verify double precision trig table using MPFR:\r\n");

    mismatch = 0;
    for (len = (1 << i); len > 0; i--, len >>= 1) {
        fprintf(stdout, "%9d: ", len);
        fflush(stdout);

        if (!table) {
            size_t buffer_len = len * sizeof(*table);
            if (buffer_len < (size_t) len) {
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
            table = (ffts_cpx_64f*) ffts_aligned_malloc(buffer_len);
            if (!table) { 
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
        }

        ffts_generate_cosine_sine_pow2_64f(table, len);
        hash = fnv_32a_buf(table, len * sizeof(*table), FNV1A_32_INIT);

#ifdef _OPENMP
#pragma omp parallel for shared(mismatch)
#endif
        for (j = 1; j <= len/2; j++) {
            double c, s;
#ifdef _OPENMP
            int tid = omp_get_thread_num();
#else
            int tid = 0;
#endif
            if (mismatch)
                continue;
            mpfr_mul_si(rop[tid], pi, -j, MPFR_RNDN);
            mpfr_div_ui(rop[tid], rop[tid], 2 * len, MPFR_RNDN);
            mpfr_sin_cos(sop[tid], cop[tid], rop[tid], MPFR_RNDN);
            c = mpfr_get_d(cop[tid], MPFR_RNDN);
            s = mpfr_get_d(sop[tid], MPFR_RNDN);
            if (distance_between_doubles(c, table[j][0]) || distance_between_doubles(s, table[j][1])) {
                mismatch = 1;
            }
        }

        fprintf(stdout, "%s (checksum: %u)\r\n", mismatch ? "incorrect" : "ok", hash);
        if (mismatch)
            break;
    }

    free(cop);
    free(rop);
    free(sop);
    return mismatch;
}
#else
/* generated using MPFR */
static const uint32_t cosine_sine_pow2_32f_checksum[MAX_LEN+1] = {
    4163796632, /*       1 */
    752475004,  /*       2 */
    3588569144, /*       4 */
    637367864,  /*       8 */
    2237388720, /*      16 */
    1881651208, /*      32 */
    356152060,  /*      64 */
    3819835580, /*      128 */
    1180132788, /*      256 */
    1716628348, /*      512 */
    2422051204, /*      1024 */
    3873830812, /*      2048 */
    1376829016, /*      4096 */
    2571876996, /*      8192 */
    759289276,  /*     16384 */
    3423626720, /*     32768 */
    3366809140, /*     65536 */
    674354300,  /*    131072 */
    3992680156, /*    262144 */
    1545599748, /*    524288 */
    586444388,  /*   1048576 */
    2907181276, /*   2097152 */
    3274911900, /*   4194304 */
    1532398892, /*   8388608 */
    2501994432, /*  16777216 */
    1992951192, /*  33554432 */
    2929058012, /*  67108864 */
    2962003792, /* 134217728 */
    934675412,  /* 268435456 */
    673507348   /* 536870912 */
};

static const uint32_t cosine_sine_pow2_64f_checksum[MAX_LEN+1] = {
    2244568568, /*       1 */
    2674442040, /*       2 */
    637646452,  /*       4 */
    365964188,  /*       8 */
    492049596,  /*      16 */
    1537563784, /*      32 */
    2795637248, /*      64 */
    1166550632, /*      128 */
    2310849600, /*      256 */
    2701603684, /*      512 */
    766126000,  /*      1024 */
    3596199488, /*      2048 */
    3189251260, /*      4096 */
    2366118336, /*      8192 */
    3948192124, /*     16384 */
    789942316,  /*     32768 */
    4160790692, /*     65536 */
    1400150244, /*    131072 */
    3473642232, /*    262144 */
    3445250900, /*    524288 */
    401920132,  /*   1048576 */
    800848444,  /*   2097152 */
    1239474840, /*   4194304 */
    3699857428, /*   8388608 */
    2185444492, /*  16777216 */
    2780078060, /*  33554432 */
    1398729992, /*  67108864 */
    372663520,  /* 134217728 */
    254558992,  /* 268435456 */
    2451140808  /* 536870912 */
};

static int test_float(int i)
{
    ffts_cpx_32f *table = NULL;
    int len, mismatch = 0;
    uint32_t hash;

    fprintf(stdout, "Verify single precision trig table using FNV-1a checksum:\r\n");

    for (len = (1 << i); len > 0; i--, len >>= 1) {
        fprintf(stdout, "%9d: ", len);
        fflush(stdout);

        if (!table) {
            size_t buffer_len = len * sizeof(*table);
            if (buffer_len < (size_t) len) {
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
            table = (ffts_cpx_32f*) ffts_aligned_malloc(buffer_len);
            if (!table) { 
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
        }

        ffts_generate_cosine_sine_pow2_32f(table, len);
        hash = fnv_32a_buf(table, len * sizeof(*table), FNV1A_32_INIT);
        mismatch = (hash != cosine_sine_pow2_32f_checksum[i]);
        fprintf(stdout, "%s\r\n", mismatch ? "incorrect" : "ok");
        if (mismatch)
            break;

    }

    ffts_aligned_free(table);
    return mismatch;
}

static int test_double(int i)
{
    ffts_cpx_64f *table = NULL;
    int len, mismatch = 0;
    uint32_t hash;

    fprintf(stdout, "Verify double precision trig table using FNV-1a checksum:\r\n");

    for (len = (1 << i); len > 0; i--, len >>= 1) {
        fprintf(stdout, "%9d: ", len);
        fflush(stdout);

        if (!table) {
            size_t buffer_len = len * sizeof(*table);
            if (buffer_len < (size_t) len) {
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
            table = (ffts_cpx_64f*) ffts_aligned_malloc(buffer_len);
            if (!table) { 
                fprintf(stdout, "out of memory\r\n");
                continue;
            }
        }

        ffts_generate_cosine_sine_pow2_64f(table, len);
        hash = fnv_32a_buf(table, len * sizeof(*table), FNV1A_32_INIT);
        mismatch = (hash != cosine_sine_pow2_64f_checksum[i]);
        fprintf(stdout, "%s\r\n", mismatch ? "incorrect" : "ok");
        if (mismatch)
            break;

    }

    ffts_aligned_free(table);
    return mismatch;
}
#endif

int main(int argc, char *argv[])
{
    int i;

    if (argc == 2) {
        i = atoi(argv[1]);
        if (i > MAX_LEN)
            i = MAX_LEN;
    } else {
        i = MAX_LEN;
    }

    test_float(i);
    test_double(i);
    return 0;
}
