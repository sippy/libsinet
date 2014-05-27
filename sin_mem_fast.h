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

