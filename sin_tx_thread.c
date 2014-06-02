#include <sys/ioctl.h>
#include <net/netmap_user.h>
#ifdef SIN_DEBUG
#include <assert.h>
#endif
#include <errno.h>
#include <signal.h>
#ifdef SIN_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "sin_list.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_stance.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
#include "sin_mem_fast.h"
#include "sin_pkt_zone_fast.h"
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
    struct sin_stance *sip;
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
    plp->head = plp->tail = (void *)pkt_zone->first[curidx];
    plp->len = 1;
    for (; ntx > 1; ntx--) {
        curidx = nm_ring_next(ring, curidx);
        sin_list_append(plp, pkt_zone->first[curidx]);
    }
}

static void
advance_tx_ring(struct netmap_ring *ring, unsigned int ntx)
{
    unsigned int curidx;

#ifdef SIN_DEBUG
     assert(ntx < ring->num_slots);
#endif

#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
     printf("advance_tx_ring: enter: ring->head = %u, ring->cur = %u,"
       "ring->tail = %u\n", ring->head, ring->cur, ring->tail);
#endif
    curidx = ring->cur;
    curidx += ntx;
    while (curidx >= ring->num_slots) {
        curidx -= ring->num_slots;
    }
    ring->head = ring->cur = curidx;
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
    printf("advance_tx_ring: exit: ring->head = %u, ring->cur = %u, "
      "ring->tail = %u\n", ring->head, ring->cur, ring->tail);
#endif
}

static void
sin_tx_thread(struct sin_tx_thread *sttp)
{
    struct netmap_ring *tx_ring;
    struct sin_pkt_zone *tx_zone;
    struct sin_list pkts_out, pkts_t;
    struct sin_pkt *pkt, *pkt_next, *pkt_out;
    unsigned int ntx, i;

    tx_ring = sttp->sip->tx_ring;
    tx_zone = sttp->sip->tx_free;
    SIN_LIST_RESET(&pkts_out);
    for (;;) {
        ioctl(sttp->sip->netmap_fd, NIOCTXSYNC, NULL);
        ntx = tx_ring_nslots(tx_ring);
        if (ntx == 0) {
            goto nextcycle;
        }
        sin_wi_queue_get_items(sttp->outpkt_queue, &pkts_out, 1, 1);
        if (!SIN_LIST_IS_EMPTY(&pkts_out)) {
            if (ntx > pkts_out.len) {
                ntx = pkts_out.len;
            }
            tx_zone_getpkts(tx_ring, tx_zone, &pkts_t, ntx);
            pkt = SIN_LIST_HEAD(&pkts_out);
            pkt_out = SIN_LIST_HEAD(&pkts_t);
            for (i = 0; i < ntx; i++) {
		if (tx_zone->netmap_fd == pkt->my_zone->netmap_fd) {
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
                    printf("zero-copying %p to %p\n", pkt, pkt_out);
#endif
                    sin_pkt_zone_swap(pkt, pkt_out);
                } else {
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
                    printf("copying %p to %p\n", pkt, pkt_out);
#endif
                    sin_pkt_zone_copy(pkt, pkt_out);
                }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
                printf("sin_tx_thread: sending %p packet of length %u out\n",
                  pkt_out, pkt_out->len);
                sin_ip4_icmp_debug(pkt_out);
#endif
                pkt_next = SIN_ITER_NEXT(pkt);
                pkt->t.sin_next = NULL;
                sin_pkt_zone_ret_pkt(pkt);
                pkt = pkt_next;
                pkt_next = SIN_ITER_NEXT(pkt_out);
                pkt_out->t.sin_next = NULL;
                pkt_out = pkt_next;
            }
            advance_tx_ring(tx_ring, ntx);
            pkts_out.head = (void *)pkt;
            if (pkt == NULL) {
                pkts_out.tail = NULL;
                pkts_out.len = 0;
            } else {
                pkts_out.len -= ntx;
            }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
            printf("sin_tx_thread: %d packets returned\n", ntx);
#endif
        }
nextcycle:
        if (sin_wrk_thread_check_ctrl(&sttp->t) == SIGTERM) {
            break;
        }
    }
}

struct sin_tx_thread *
sin_tx_thread_ctor(struct sin_stance *sip, int *e)
{
    struct sin_tx_thread *sttp;

    sttp = malloc(sizeof(struct sin_tx_thread));
    if (sttp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(sttp, '\0', sizeof(struct sin_tx_thread));
    sttp->outpkt_queue = sin_wi_queue_ctor(e, "tx_thread out packets queue");
    if (sttp->outpkt_queue == NULL) {
        goto er_undo_1;
    }
    sttp->sip = sip;
    if (sin_wrk_thread_ctor(&sttp->t, "tx_thread #0",
      (void *(*)(void *))&sin_tx_thread, e) != 0) {
        goto er_undo_2;
    }
    sin_wrk_thread_notify_on_ctrl(&sttp->t, sttp->outpkt_queue);

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
    sin_wrk_thread_dtor(&sttp->t);
    sin_wi_queue_dtor(sttp->outpkt_queue);
    free(sttp);
}
