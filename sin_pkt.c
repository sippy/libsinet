#include <sys/types.h>
#include <net/netmap_user.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "sin_errno.h"
#include "sin_pkt.h"

struct sin_pkt *
sin_pkt_ctor(struct sin_pkt_zone *my_zone, int zone_idx,
 struct netmap_ring *my_ring, int *e)
{
    struct sin_pkt *pkt;

    pkt = malloc(sizeof(struct sin_pkt));
    if (pkt == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(pkt, '\0', sizeof(struct sin_pkt));
    SIN_TYPE_SET(pkt, _SIN_TYPE_PKT);
    pkt->my_zone = my_zone;
    pkt->my_ring = my_ring;
    pkt->zone_idx = zone_idx;
    pkt->my_slot = &(my_ring->slot[zone_idx]);
    pkt->buf = NETMAP_BUF(my_ring, pkt->my_slot->buf_idx);

    return (pkt);
}


void
sin_pkt_dtor(struct sin_pkt *pkt)
{

    SIN_TYPE_ASSERT(pkt, _SIN_TYPE_PKT);
    free(pkt);
}
