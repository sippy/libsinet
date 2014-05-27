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

