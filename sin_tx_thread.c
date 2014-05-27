#include <net/netmap_user.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
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

struct sin_tx_thread
{
    struct sin_type_wrk_thread t;
    struct sin_wi_queue *outpkt_queue;
    struct sin_stance *sip;
};

static inline struct sin_pkt *
advance_tx_ring(struct netmap_ring *ring, struct sin_pkt_zone *pkt_zone)
{
    struct sin_pkt *pkt;
    unsigned int curidx;

    curidx = ring->cur;
    if (ring->cur == ring->tail) {
        return (NULL);
    }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
     printf("advance_tx_ring: enter: ring->head = %u, ring->cur = %u, ring->tail = %u\n",
       ring->head, ring->cur, ring->tail);
#endif
    pkt = pkt_zone->first[curidx];
#ifdef SIP_DEBUG
    assert(pkt->buf == NETMAP_BUF(ring, ring->slot[curidx].buf_idx));
#endif
    pkt->len = ring->slot[curidx].len;
    ring->head = ring->cur = nm_ring_next(ring, curidx);
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
     printf("advance_tx_ring: exit: ring->head = %u, ring->cur = %u, ring->tail = %u\n",
       ring->head, ring->cur, ring->tail);
#endif
    return (pkt);
}

static void
sin_tx_thread(struct sin_tx_thread *sttp)
{
    struct netmap_ring *tx_ring;
    struct pollfd fds;
    struct sin_list pkts_out;
    struct sin_pkt *pkt, *pkt_next, *pkt_out;
    int nready;
    unsigned int nconsumed;

    tx_ring = sttp->sip->tx_ring;
    fds.fd = sttp->sip->netmap_fd;
    fds.events = POLLOUT;
    SIN_LIST_RESET(&pkts_out);
    for (;;) {
        nready = poll(&fds, 1, 10);
        if (nready > 0) {
            sin_wi_queue_get_items(sttp->outpkt_queue, &pkts_out, 1, 1);
        }
        if (!SIN_LIST_IS_EMPTY(&pkts_out)) {
            nconsumed = 0;
            pkt_next = NULL;
            for (pkt = SIN_LIST_HEAD(&pkts_out); pkt != NULL;
              pkt = pkt_next) {
                pkt_out = advance_tx_ring(tx_ring, sttp->sip->tx_free);
                if (pkt_out == NULL) {
                    break;
                }
                pkt_next = SIN_ITER_NEXT(pkt);
                pkt->t.sin_next = NULL;
#if 1
                sin_pkt_zone_swap(pkt, pkt_out);
#else
                sin_pkt_zone_copy(pkt, pkt_out);
#endif
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
                printf("sin_tx_thread: sending %p packet of length %u out\n", pkt_out, pkt_out->len);
#endif
                sin_pkt_zone_ret_pkt(pkt, pkt->my_zone);
                nconsumed++;
            }
            pkts_out.head = (void *)pkt;
            if (pkt == NULL) {
                pkts_out.tail = NULL;
                pkts_out.len = 0;
            } else {
                pkts_out.len -= nconsumed;
            }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
            if (nconsumed > 0) {
                printf("sin_tx_thread: %d packets returned\n", nconsumed);
            }
#endif
        }
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
