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

#include <sys/ioctl.h>
#include <net/netmap_user.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "sin_types.h"
#include "sin_debug.h"
#include "sin_list.h"
#include "sin_errno.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
#include "sin_math.h"
#include "sin_mem_fast.h"
#include "sin_pkt_zone_fast.h"
#include "sin_stance.h"
#include "sin_wi_queue.h"
#include "sin_wrk_thread.h"
#include "sin_tx_thread.h"

#ifdef SIN_DEBUG
#include "sin_ip4_icmp.h"
#endif

struct sin_tx_thread
{
    struct sin_type_wrk_thread t;
    struct sin_wi_queue *outpkt_queue;
    struct netmap_ring *tx_ring;
    struct sin_pkt_zone *tx_zone;
    int queue_fd;
};

static unsigned int
tx_ring_nslots(struct netmap_ring *ring)
{

    if (ring->cur > ring->tail) {
        return (ring->num_slots + ring->tail - ring->cur);
    }
    return (ring->tail - ring->cur);
}

static void
tx_zone_getpkts(struct netmap_ring *ring, struct sin_pkt_zone *pkt_zone,
  struct sin_list *plp, unsigned int ntx)
{
    unsigned int curidx;

    curidx = ring->cur;
    plp->head = plp->tail = (void *)pkt_zone->pmap[curidx];
    SPKT_DBG_TRACE(pkt_zone->pmap[curidx]);
    plp->len = 1;
    for (; ntx > 1; ntx--) {
        curidx = nm_ring_next(ring, curidx);
        sin_list_append(plp, pkt_zone->pmap[curidx]);
        SPKT_DBG_TRACE(pkt_zone->pmap[curidx]);
    }
}

static void
advance_tx_ring(struct netmap_ring *ring, unsigned int ntx)
{
    unsigned int curidx;

    SIN_DEBUG_ASSERT(ntx < ring->num_slots);

    curidx = ring->cur;
    curidx += ntx;
    while (curidx >= ring->num_slots) {
        curidx -= ring->num_slots;
    }
    ring->head = ring->cur = curidx;
}

#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
static void
_advance_tx_ring(const char *tname, struct netmap_ring *ring, unsigned int ntx)
{
    printf("%s: advance_tx_ring: enter: ring->head = %u, ring->cur = %u,"
       "ring->tail = %u\n", tname, ring->head, ring->cur, ring->tail);
    advance_tx_ring(ring, ntx);
    printf("%s: advance_tx_ring: exit: ring->head = %u, ring->cur = %u, "
      "ring->tail = %u\n", tname, ring->head, ring->cur, ring->tail);
}
#define advance_tx_ring(a, b, c) _advance_tx_ring(a, b, c)
#else
#define advance_tx_ring(a, b, c) advance_tx_ring(b, c)
#endif

static void
sin_tx_thread(struct sin_tx_thread *sttp)
{
    struct netmap_ring *tx_ring;
    struct sin_pkt_zone *tx_zone;
    struct sin_list pkts_out, pkts_tx;
    struct sin_pkt *pkt, *pkt_next, *pkt_tx;
    unsigned int ntx, i, atx, aslots;
    const char *tname;

    tname = CALL_METHOD(&sttp->t, get_tname);

    tx_ring = sttp->tx_ring;
    tx_zone = sttp->tx_zone;
    SIN_LIST_RESET(&pkts_out);
    for (;;) {
        aslots = tx_ring_nslots(tx_ring);
        atx = 0;
        if (aslots == 0) {
            goto nextcycle;
        }
        if (SIN_LIST_IS_EMPTY(&pkts_out)) {
            sin_wi_queue_get_items(sttp->outpkt_queue, &pkts_out, 1, 1);
        }
        if (!SIN_LIST_IS_EMPTY(&pkts_out)) {
            ntx = MIN(aslots, pkts_out.len);
            tx_zone_getpkts(tx_ring, tx_zone, &pkts_tx, ntx);
            pkt = SIN_LIST_HEAD(&pkts_out);
            pkt_tx = SIN_LIST_HEAD(&pkts_tx);
            for (i = 0; i < ntx; i++) {
                SPKT_DBG_TRACE(pkt);
		if (tx_zone->netmap_fd == pkt->my_zone->netmap_fd) {
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
                    printf("%s: zero-copying %p to %p\n", tname, pkt, pkt_tx);
#endif
                    sin_pkt_zone_swap(pkt, pkt_tx);
                } else {
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
                    printf("%s: copying %p to %p\n", tname, pkt, pkt_tx);
#endif
                    sin_pkt_zone_copy(pkt, pkt_tx);
                }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 4)
                printf("%s: sin_tx_thread: sending %p packet of length %u out\n",
                  tname, pkt_tx, pkt_tx->len);
                if (sin_ip4_icmp_repl_taste(pkt_tx)) {
                    sin_ip4_icmp_debug(pkt_tx);
                }
#endif
                pkt_next = SIN_ITER_NEXT(pkt);
                pkt->t.sin_next = NULL;
                SPKT_DBG_TRACE(pkt);
                sin_pkt_zone_ret_pkt(pkt);
                pkt = pkt_next;
                pkt_next = SIN_ITER_NEXT(pkt_tx);
                pkt_tx->t.sin_next = NULL;
                pkt_tx = pkt_next;
            }
            advance_tx_ring(tname, tx_ring, ntx);
            atx += ntx;
            pkts_out.head = (void *)pkt;
            if (pkt == NULL) {
                pkts_out.tail = NULL;
                SIN_DEBUG_ASSERT(pkts_out.len == ntx);
                pkts_out.len = 0;
            } else {
                pkts_out.len -= ntx;
            }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
            printf("%s: sin_tx_thread: %d packets returned\n", tname, ntx);
#endif
        }
nextcycle:
        if (atx > 0 || aslots == 0) {
            ioctl(sttp->queue_fd, NIOCTXSYNC, NULL);
        }
        if (CALL_METHOD(&sttp->t, check_ctrl) == SIGTERM) {
            break;
        }
        if (aslots == 0) {
            usleep(1000);
        }
    }
}

struct sin_tx_thread *
sin_tx_thread_ctor(const char *tname, struct wrk_set *wsp, int *e)
{
    struct sin_tx_thread *sttp;

    sttp = malloc(sizeof(struct sin_tx_thread));
    if (sttp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(sttp, '\0', sizeof(struct sin_tx_thread));
    sttp->outpkt_queue = sin_wi_queue_ctor(e, "tx_thread(%s) out packets queue",
      tname);
    if (sttp->outpkt_queue == NULL) {
        goto er_undo_1;
    }
    sttp->tx_ring = wsp->tx_ring;
    sttp->tx_zone = wsp->tx_zone;
    sttp->queue_fd = wsp->queue_fd;
    if (sin_wrk_thread_ctor(&sttp->t, tname,
      (void *(*)(void *))&sin_tx_thread, e) != 0) {
        goto er_undo_2;
    }
    CALL_METHOD(&sttp->t, notify_on_ctrl, sttp->outpkt_queue);

    return (sttp);

er_undo_2:
    sin_wi_queue_dtor(sttp->outpkt_queue);
er_undo_1:
    free(sttp);
    return (NULL);
}

struct sin_wi_queue *
sin_tx_thread_get_out_queue(struct sin_tx_thread *sttp)
{

    return (sttp->outpkt_queue);
}

void
sin_tx_thread_dtor(struct sin_tx_thread *sttp)
{

    SIN_TYPE_ASSERT(sttp, _SIN_TYPE_WRK_THREAD);
    CALL_METHOD(&sttp->t, dtor);
    sin_wi_queue_dtor(sttp->outpkt_queue);
    free(sttp);
}
