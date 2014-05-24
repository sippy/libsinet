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
#include "sin_errno.h"
#include "sin_stance.h"
#include "sin_pkt.h"
#include "sin_pkt_zone.h"
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

static void
sin_tx_thread(struct sin_tx_thread *sttp)
{
    struct netmap_ring *tx_ring;
    struct pollfd fds;
    struct sin_list pkts_out;
    struct sin_pkt *pkt, *pkt_next;
    int nready;

    tx_ring = sttp->sip->tx_ring;
    fds.fd = sttp->sip->netmap_fd;
    fds.events = POLLOUT;
    SIN_LIST_RESET(&pkts_out);
    for (;;) {
        nready = poll(&fds, 1, 10);
        if (nready > 0) {
            if (sin_wi_queue_get_items(sttp->outpkt_queue, &pkts_out, 1, 1) != NULL) {
                pkt_next = NULL;
                for (pkt = SIN_LIST_HEAD(&pkts_out); pkt != NULL; pkt = pkt_next) {
                    pkt_next = SIN_ITER_NEXT(pkt);
                    pkt->t.sin_next = NULL;
                    sin_pkt_zone_ret_pkt(pkt, sttp->sip->rx_free);
                }
#ifdef SIN_DEBUG
		printf("sin_tx_thread: %d packets returned\n", pkts_out.len);
#endif
                SIN_LIST_RESET(&pkts_out);
            }
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
