#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_pkt_zone.h"

struct sin_pkt;

struct sin_pkt_zone {
    unsigned int sin_type;
    struct sin_pkt **first;
    struct sin_pkt **last;
    struct sin_pkt **curr;
};

struct sin_pkt_zone *
sin_pkt_zone_ctor(int zone_len, int *e)
{
    struct sin_pkt_zone *spzp;

    spzp = malloc(sizeof(struct sin_pkt_zone) + (sizeof(struct sin_pkt *) * zone_len));
    if (spzp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    spzp->first = malloc(sizeof(struct sin_pkt *) * zone_len);
    if (spzp->first == NULL) {
        free(spzp);
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    spzp->sin_type = _SIN_TYPE_PKT_ZONE;
    spzp->curr = spzp->first - 1;
    spzp->last = spzp->first + zone_len - 1;

    return (spzp);
}

void
sin_pkt_zone_dtor(struct sin_pkt_zone *spzp)
{

    SIN_TYPE_ASSERT(spzp, _SIN_TYPE_PKT_ZONE);
    free(spzp->first);
    free(spzp);
}
