#include <sys/types.h>

#include "sin_mem_fast.h"

int main()
{
    uint8_t a[6], b[6];

    memswp(a, b, 6);
    return (0);
}
