#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "sin_errno.h"
#include "sin_signal.h"

struct sin_signal
{
    struct sin_type_linkable t;
    int signum;
};

struct sin_signal *
sin_signal_ctor(int signum, int *e)
{
    struct sin_signal *ssign;

    ssign = malloc(sizeof(struct sin_signal));
    if (ssign == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(ssign, '\0', sizeof(struct sin_signal));
    SIN_TYPE_SET(ssign, _SIN_TYPE_SIGNAL);
    ssign->signum = signum;

    return (ssign);
}


void
sin_signal_dtor(struct sin_signal *ssign)
{

    SIN_TYPE_ASSERT(ssign, _SIN_TYPE_SIGNAL);
    free(ssign);
}

int
sin_signal_get_signum(struct sin_signal *ssign)
{

    SIN_TYPE_ASSERT(ssign, _SIN_TYPE_SIGNAL);
    return (ssign->signum);
}
