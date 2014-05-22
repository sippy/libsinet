#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_pkt.h"

struct sin_pkt
{
    int sin_type;
    struct netmap_slot *nm_slot;
};

struct sin_pkt *
sin_pkt_ctor(struct netmap_slot *nm_slot, int *e)
{
    struct sin_pkt *pkt;

    pkt = malloc(sizeof(struct sin_pkt));
    if (pkt == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    pkt->sin_type = _SIN_TYPE_PKT;
    pkt->nm_slot = nm_slot;

    return (pkt);
}


void
sin_pkt_dtor(struct sin_pkt *pkt)
{

    SIN_TYPE_ASSERT(pkt, _SIN_TYPE_PKT);
    free(pkt);
}

