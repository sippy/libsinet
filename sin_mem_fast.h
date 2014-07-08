/*
 * Copyright (c) 2014 Sippy Software, Inc., http://www.sippysoft.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef likely
#define _sin_likely(x)       __builtin_expect(!!(x), 1)
#define _sin_unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define _sin_likely(x)	     likely(x)
#define _sin_unlikely(x)     unlikely(x)
#endif /* sin_likely and sin_unlikely */

static inline void
sin_memswp(unsigned char *r1, unsigned char *r2, unsigned int len)
{
    unsigned int i;

    for (i = 0; i < len; ++i) {
        r1[i] = r1[i] ^ r2[i];
        r2[i] = r1[i] ^ r2[i];
        r1[i] = r1[i] ^ r2[i];
    }
}

static inline void
sin_memcpy(const void *_src, void *_dst, int l)
{
    const uint64_t *src = (const uint64_t *)_src;
    uint64_t *dst = (uint64_t *)_dst;

    if (_sin_unlikely(l >= 1024)) {
        memcpy(dst, src, l);
        return;
    }
    for (; _sin_likely(l > 0); l-=64) {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
    }
}

#define _SIN_SWAP_I(a, b) {a = a ^ b; b = a ^ b; a = a ^ b;}
#define _SIN_SWAP_P(a, b) _SIN_SWAP_I(*(uintptr_t *)(&a), *(uintptr_t *)(&b))

