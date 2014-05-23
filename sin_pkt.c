#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_pkt.h"

struct sin_pkt
{
    struct sin_type_linkable t;
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
    memset(pkt, '\0', sizeof(struct sin_pkt));
    SIN_TYPE_SET(pkt, _SIN_TYPE_PKT);
    pkt->nm_slot = nm_slot;

    return (pkt);
}


void
sin_pkt_dtor(struct sin_pkt *pkt)
{

    SIN_TYPE_ASSERT(pkt, _SIN_TYPE_PKT);
    free(pkt);
}
