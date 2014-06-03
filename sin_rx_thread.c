#include <sys/ioctl.h>
#include <net/netmap_user.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#ifdef SIN_DEBUG
#include <assert.h>
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_list.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
#include "sin_wrk_thread.h"
#include "sin_rx_thread.h"
#include "sin_pkt_sorter.h"

struct sin_rx_thread
{
    struct sin_type_wrk_thread t;
    struct netmap_ring *rx_ring;
    struct sin_pkt_zone *rx_zone;
    struct sin_pkt_sorter *rx_sort;
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
    struct pollfd fds;
    int nready, ndeq;

    tname = sin_wrk_thread_get_tname(&srtp->t);
    fds.fd = srtp->rx_zone->netmap_fd;
    fds.events = POLLIN;
    SIN_LIST_RESET(&pkts_in);
    for (;;) {
        nready = poll(&fds, 1, 10);
        if (nready > 0 && !nm_ring_empty(srtp->rx_ring)) {
            ndeq = dequeue_pkts(srtp->rx_ring, srtp->rx_zone, &pkts_in);
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 4)
            printf("%s: dequeued %d packets\n", tname, ndeq);
#endif
            if (!SIN_LIST_IS_EMPTY(&pkts_in)) {
                sin_pkt_sorter_proc(srtp->rx_sort, &pkts_in);
                SIN_LIST_RESET(&pkts_in);
            }
        }
        if (sin_wrk_thread_check_ctrl(&srtp->t) == SIGTERM) {
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
    srtp->rx_ring = rx_ring;
    srtp->rx_zone = rx_zone;
    srtp->rx_sort = rx_sort;

    if (sin_wrk_thread_ctor(&srtp->t, tname,
      (void *(*)(void *))&sin_rx_thread, e) != 0) {
        free(srtp);
        return (NULL);
    }
    return (srtp);
}

void
sin_rx_thread_dtor(struct sin_rx_thread *srtp)
{

    SIN_TYPE_ASSERT(srtp, _SIN_TYPE_WRK_THREAD);
    sin_wrk_thread_dtor(&srtp->t);
    free(srtp);
}
