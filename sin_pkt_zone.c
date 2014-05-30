#include <sys/types.h>
#include <net/netmap_user.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "sin_type.h"
#include "sin_errno.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"

struct sin_pkt;
struct sin_pkt_zone_pvt {
    pthread_mutex_t mutex;
};

static int
sin_pkt_zone_fill_from_ring(struct sin_pkt_zone *spzp,
  struct netmap_ring *ring, int *e)
{
    int i, j;
    struct sin_pkt *pkt, **pkts;

    pkts = spzp->first;
    for (i = 0; i < (int)ring->num_slots; i++)  {
        pkt = sin_pkt_ctor(spzp, i, ring, e);
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
    int eval;

    spzp = malloc(sizeof(struct sin_pkt_zone));
    if (spzp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    spzp->pvt = malloc(sizeof(struct sin_pkt_zone_pvt));
    if (spzp->pvt == NULL) {
        _SET_ERR(e, ENOMEM);
        goto er_undo_0;
    }
    eval = pthread_mutex_init(&spzp->pvt->mutex, NULL);
    if (eval != 0) {
        _SET_ERR(e, eval);
        goto er_undo_1;
    }
    spzp->first = malloc(sizeof(struct sin_pkt *) * ring->num_slots);
    if (spzp->first == NULL) {
        _SET_ERR(e, ENOMEM);
        goto er_undo_2;
    }
    if (sin_pkt_zone_fill_from_ring(spzp, ring, e) < 0) {
        goto er_undo_2;
    }
    SIN_TYPE_SET(spzp, _SIN_TYPE_PKT_ZONE);
    spzp->last = spzp->first + ring->num_slots - 1;

    return (spzp);

er_undo_2:
    pthread_mutex_destroy(&spzp->pvt->mutex);
er_undo_1:
    free(spzp->pvt);
er_undo_0:
    free(spzp);
    return (NULL);
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
    pthread_mutex_destroy(&spzp->pvt->mutex);
    free(spzp->pvt);
    free(spzp);
}

int
sin_pkt_zone_lock(struct sin_pkt_zone *spzp)
{

    return (pthread_mutex_lock(&spzp->pvt->mutex));
}

int
sin_pkt_zone_unlock(struct sin_pkt_zone *spzp)
{

    return (pthread_mutex_unlock(&spzp->pvt->mutex));
}
