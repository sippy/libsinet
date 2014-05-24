#include <sys/types.h>
#include <net/netmap_user.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "sin_type.h"
#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"

struct sin_pkt;

static int
sin_pkt_zone_fill_from_ring(struct sin_pkt_zone *spzp,
  struct netmap_ring *ring, int *e)
{
    int i, j;
    struct sin_pkt *pkt, **pkts;

    pkts = spzp->first;
    for (i = 0; i < (int)ring->num_slots; i++)  {
        pkt = sin_pkt_ctor(spzp, i, e);
        if (pkt == NULL) {
            for (j = i - 1; j >= 0; j--) {
                sin_pkt_dtor(pkts[j]);
            }
            return (-1);
        }
        pkts[i] = pkt;
    }
    return (i);
}

struct sin_pkt_zone *
sin_pkt_zone_ctor(struct netmap_ring *ring, int *e)
{
    struct sin_pkt_zone *spzp;

    spzp = malloc(sizeof(struct sin_pkt_zone));
    if (spzp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    spzp->first = malloc(sizeof(struct sin_pkt *) * ring->num_slots);
    if (spzp->first == NULL) {
        free(spzp);
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    if (sin_pkt_zone_fill_from_ring(spzp, ring, e) < 0) {
        free(spzp);
        return (NULL);
    }
    SIN_TYPE_SET(spzp, _SIN_TYPE_PKT_ZONE);
    spzp->curr = spzp->first - 1;
    spzp->last = spzp->first + ring->num_slots - 1;

    return (spzp);
}

void
sin_pkt_zone_dtor(struct sin_pkt_zone *spzp)
{
    struct sin_pkt **pkt;

    SIN_TYPE_ASSERT(spzp, _SIN_TYPE_PKT_ZONE);
    for (pkt = spzp->first; pkt <= spzp->last; pkt++) {
        free(*pkt);
    }
    free(spzp->first);
    free(spzp);
}
