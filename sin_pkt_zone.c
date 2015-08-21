/*
 * Copyright (c) 2014 Sippy Software, Inc., http://www.sippysoft.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <net/netmap_user.h>
#ifdef SIN_DEBUG
#include <assert.h>
#endif
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "sin_types.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
#include "sin_mem_fast.h"
#include "sin_pkt_zone_fast.h"
#include "sin_list.h"

struct sin_pkt;

struct sin_pkt_zone_pvt {
    int num_slots;
    pthread_mutex_t mutex;
};

static int
sin_pkt_zone_fill_from_ring(struct sin_pkt_zone *spzp,
  struct netmap_ring *ring, int *e)
{
    int i, j;
    struct sin_pkt *pkt, **pkts;

    pkts = spzp->pmap;
    for (i = 0; i < (int)ring->num_slots; i++)  {
        pkt = sin_pkt_ctor(spzp, i, ring, e);
        if (pkt == NULL) {
            for (j = i - 1; j >= 0; j--) {
                sin_pkt_dtor(pkts[j]);
            }
            return (-1);
        }
        SPKT_DBG_TRACE(pkt);
        pkts[i] = pkt;
    }
    return (i);
}

struct sin_pkt_zone *
sin_pkt_zone_ctor(struct netmap_ring *ring, int netmap_fd, int *e)
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
    spzp->pmap = malloc(sizeof(struct sin_pkt *) * ring->num_slots);
    if (spzp->pmap == NULL) {
        _SET_ERR(e, ENOMEM);
        goto er_undo_2;
    }
    if (sin_pkt_zone_fill_from_ring(spzp, ring, e) < 0) {
        goto er_undo_2;
    }
    SIN_TYPE_SET(spzp, _SIN_TYPE_PKT_ZONE);
    spzp->netmap_fd = netmap_fd;
    spzp->pvt->num_slots = ring->num_slots;

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
    int i;

    SIN_TYPE_ASSERT(spzp, _SIN_TYPE_PKT_ZONE);
    for (i = 0; i < spzp->pvt->num_slots; i++) {
        free(spzp->pmap[i]);
    }
    free(spzp->pmap);
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

void
#ifdef SIN_DEBUG
sin_pkt_zone_ret_all(struct sin_list *pl, void *arg)
#else
sin_pkt_zone_ret_all(struct sin_list *pl, __attribute__((unused))void *arg)
#endif
{
    struct sin_pkt *pkt, *pkt_next;
#ifdef SIN_DEBUG
    struct sin_pkt_zone *zone;

    zone = (struct sin_pkt_zone *)arg;
#endif

    for (pkt = SIN_LIST_HEAD(pl); pkt != NULL; pkt = pkt_next) {
#ifdef SIN_DEBUG
        assert(zone == pkt->my_zone);
#endif
        pkt_next = SIN_ITER_NEXT(pkt);
        SIN_TYPE_LINK(pkt, NULL);
        sin_pkt_zone_ret_pkt(pkt);
    }
}
