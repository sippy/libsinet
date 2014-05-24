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
#include "sin_errno.h"
#include "sin_list.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
#include "sin_stance.h"
#include "sin_wi_queue.h"
#include "sin_wrk_thread.h"
#include "sin_rx_thread.h"
#include "sin_ip4_icmp.h"

struct sin_rx_thread
{
    struct sin_type_wrk_thread t;
    struct sin_wi_queue *inpkt_queue;
    struct sin_stance *sip;
};

static struct sin_pkt *
get_nextpkt(struct netmap_ring *ring, struct sin_pkt_zone *pzone)
{
     int i, idx;
     struct sin_pkt *pkt;

     if (nm_ring_empty(ring)) {
         /* Nothing found */
         return (NULL);
     }
     i = ring->cur;
     pkt = pzone->first[i];
     assert(pkt->zone_idx == i);
     pzone->first[i] = NULL;
     idx = ring->slot[i].buf_idx;

     pkt->buf = (u_char *)NETMAP_BUF(ring, idx);
     pkt->ts = ring->ts;
     pkt->len = ring->slot[i].len;
     ring->cur = nm_ring_next(ring, i);
     return (pkt);
}

static inline void
spin_ring(struct netmap_ring *ring, struct sin_pkt_zone *pzone)
{
     unsigned int i, new_head;

#ifdef SIN_DEBUG
     printf("spin_ring: enter: ring->head = %u, ring->cur = %u, ring->tail = %u\n",
       ring->head, ring->cur, ring->tail);
#endif
     new_head = ring->head;
     for (i = ring->head; i != ring->cur; i = nm_ring_next(ring, i)) {
         if (pzone->first[i] == NULL) {
             break;
         }
         new_head = i;
     }
     ring->head = new_head;
#ifdef SIN_DEBUG
     printf("spin_ring: exit: ring->head = %u, ring->cur = %u, ring->tail = %u\n",
       ring->head, ring->cur, ring->tail);
#endif
}

static inline void
return_pkt(struct sin_pkt *pkt, struct sin_pkt_zone *pzone)
{

#ifdef SIN_DEBUG
    assert(pzone->first[pkt->zone_idx] == NULL);
#endif
    pzone->first[pkt->zone_idx] = pkt;
}

static void
sin_rx_thread(struct sin_rx_thread *srtp)
{
    struct netmap_ring *rx_ring;
    struct pollfd fds;
    struct sin_pkt *pkt;
    struct sin_list pkts_icmp;
    int need_spin, nready;

    rx_ring = srtp->sip->rx_ring;
    fds.fd = srtp->sip->netmap_fd;
    fds.events = POLLIN;
    SIN_LIST_RESET(&pkts_icmp);
    for (;;) {
        nready = poll(&fds, 1, 10);
        if (nready > 0) {
            need_spin = 0;
            while ((pkt = get_nextpkt(rx_ring, srtp->sip->rx_free))) {
#ifdef SIN_DEBUG
                printf("got packet, length %d, icmp = %d!\n", pkt->len,
                  sin_ip4_icmp_taste(pkt));
#endif
                if (sin_ip4_icmp_taste(pkt) == 1) {
                    sin_list_append(&pkts_icmp, pkt);
                } else {
                    need_spin = 1;
                    return_pkt(pkt, srtp->sip->rx_free);
                }
            }
            if (need_spin != 0) {
                spin_ring(rx_ring, srtp->sip->rx_free);
            }
        }
        if (sin_wrk_thread_check_ctrl(&srtp->t) == SIGTERM) {
            break;
        }
    }
}

struct sin_rx_thread *
sin_rx_thread_ctor(struct sin_stance *sip, int *e)
{
    struct sin_rx_thread *srtp;

    srtp = malloc(sizeof(struct sin_rx_thread));
    if (srtp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(srtp, '\0', sizeof(struct sin_rx_thread));
    srtp->sip = sip;
    if (sin_wrk_thread_ctor(&srtp->t, "rx_thread #0",
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
