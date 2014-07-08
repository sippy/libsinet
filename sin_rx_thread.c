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

#include <net/netmap_user.h>
#include <errno.h>
#include <signal.h>
#ifdef SIN_DEBUG
#include <assert.h>
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "sin_types.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_list.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
#include "sin_signal.h"
#include "sin_wrk_thread.h"
#include "sin_rx_thread.h"
#include "sin_pkt_sorter.h"
#include "sin_wi_queue.h"

struct sin_rx_thread
{
    struct sin_type_wrk_thread t;
    struct netmap_ring *rx_ring;
    struct sin_pkt_zone *rx_zone;
    struct sin_pkt_sorter *rx_sort;
    struct sin_wi_queue *io_notify;
    struct sin_signal *sigio;
};

static int
dequeue_pkts(struct netmap_ring *ring, struct sin_pkt_zone *pzone,
  struct sin_list *pl)
{
     unsigned int i, nrx;
     struct sin_pkt *pkt;

     nrx = 0;
     while (!nm_ring_empty(ring)) {
         i = ring->cur;
         pkt = pzone->first[i];
#ifdef SIN_DEBUG
         assert(pkt->zone_idx == i);
         assert(pkt->buf == NETMAP_BUF(ring, ring->slot[i].buf_idx));
#endif
         pzone->first[i] = NULL;
         *pkt->ts = ring->ts;
         pkt->len = ring->slot[i].len;
         ring->cur = nm_ring_next(ring, i);
         sin_list_append(pl, pkt);
         nrx++;
     }
     return (nrx);
}

#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
#define spin_ring(a, b, c) _spin_ring(a, b, c)

static inline void
_spin_ring(const char *tname, struct netmap_ring *ring,
  struct sin_pkt_zone *pzone)
#else
#define spin_ring(a, b, c) _spin_ring(b, c)

static inline void
_spin_ring(struct netmap_ring *ring,
  struct sin_pkt_zone *pzone)
#endif
{
     unsigned int i, new_head;

     if (ring->cur == nm_ring_next(ring, ring->head) ||
       (ring->head == 0 && ring->cur == 0 && ring->tail == 0)) {
         return;
     }
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
     printf("%s: spin_ring: enter: ring->head = %u, ring->cur = %u, "
       "ring->tail = %u\n", tname, ring->head, ring->cur, ring->tail);
#endif
     new_head = ring->head;
     for (i = ring->head; i != ring->cur; i = nm_ring_next(ring, i)) {
         if (pzone->first[i] == NULL) {
             break;
         }
         new_head = i;
     }
     ring->head = new_head;
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
     printf("%s: spin_ring: exit: ring->head = %u, ring->cur = %u, "
       "ring->tail = %u\n", tname, ring->head, ring->cur, ring->tail);
#endif
}

static void
sin_rx_thread(struct sin_rx_thread *srtp)
{
    struct sin_list pkts_in;
    const char *tname;
    int ndeq;
    struct sin_signal *sigio;

    tname = CALL_METHOD(&srtp->t, get_tname);
    SIN_LIST_RESET(&pkts_in);
    for (;;) {
        sigio = sin_wi_queue_get_item(srtp->io_notify, 1,  1);
        if (sigio != NULL && !nm_ring_empty(srtp->rx_ring)) {
            ndeq = dequeue_pkts(srtp->rx_ring, srtp->rx_zone, &pkts_in);
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 4)
            printf("%s: dequeued %d packets\n", tname, ndeq);
#endif
            if (!SIN_LIST_IS_EMPTY(&pkts_in)) {
                sin_pkt_sorter_proc(srtp->rx_sort, &pkts_in);
                SIN_LIST_RESET(&pkts_in);
            }
        }
        if (CALL_METHOD(&srtp->t, check_ctrl) == SIGTERM) {
            break;
        }
        spin_ring(tname, srtp->rx_ring, srtp->rx_zone);
    }
}

struct sin_rx_thread *
sin_rx_thread_ctor(const char *tname, struct netmap_ring *rx_ring,
  struct sin_pkt_zone *rx_zone, struct sin_pkt_sorter *rx_sort, int *e)
{
    struct sin_rx_thread *srtp;

    srtp = malloc(sizeof(struct sin_rx_thread));
    if (srtp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(srtp, '\0', sizeof(struct sin_rx_thread));
    srtp->io_notify = sin_wi_queue_ctor(e, "%s I/O notify queue", tname);
    if (srtp->io_notify == NULL) {
        goto er_undo_1;
    }
    srtp->sigio = sin_signal_ctor(SIGIO, e);
    if (srtp->sigio == NULL) {
        goto er_undo_2;
    }

    srtp->rx_ring = rx_ring;
    srtp->rx_zone = rx_zone;
    srtp->rx_sort = rx_sort;

    if (sin_wrk_thread_ctor(&srtp->t, tname,
      (void *(*)(void *))&sin_rx_thread, e) != 0) {
        goto er_undo_3;
    }
    CALL_METHOD(&srtp->t, notify_on_ctrl, srtp->io_notify);
    return (srtp);

er_undo_3:
    sin_signal_dtor(srtp->sigio);
er_undo_2:
    sin_wi_queue_dtor(srtp->io_notify);
er_undo_1:
    free(srtp);
    return (NULL);
}

void
sin_rx_thread_dtor(struct sin_rx_thread *srtp)
{

    SIN_TYPE_ASSERT(srtp, _SIN_TYPE_WRK_THREAD);
    CALL_METHOD(&srtp->t, dtor);
    sin_signal_dtor(srtp->sigio);
    sin_wi_queue_dtor(srtp->io_notify);
    free(srtp);
}

void
sin_rx_thread_wakeup(struct sin_rx_thread *srtp)
{
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 5)
     printf("sin_rx_thread_wakeup(%s)\n", CALL_METHOD(&srtp->t, get_tname));
#endif
     sin_wi_queue_put_item(srtp->sigio, srtp->io_notify, 1);
}
