#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "include/libsinet.h"
#include "libsinet_internal.h"
#include "sin_errno.h"

void *
sin_queue(int *e)
{
    struct sin_queue *sqp;

    sqp = malloc(sizeof(struct sin_queue));
    if (sqp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(sqp, '\0', sizeof(struct sin_queue));
    sqp->sin_type = _SIN_TYPE_QUEUE;
    return ((void *)sqp);
}
