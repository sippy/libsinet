#include <net/netmap_user.h>
#include <assert.h>
#include <errno.h>
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
#include "sin_stance.h"
#include "sin_signal.h"
#include "sin_wi_queue.h"
#include "sin_wrk_thread.h"
#include "sin_rx_thread.h"

struct sin_rx_thread
{
    struct sin_type_wrk_thread t;
    struct sin_wi_queue *inpkt_queue;
    struct sin_wi_queue *control_queue;
    struct sin_stance *sip;
};

static void
sin_rx_thread(struct sin_rx_thread *srtp)
{
    struct netmap_ring *rx_ring;
    struct sin_signal *ssign;

    rx_ring = srtp->sip->rx_ring;
#ifdef SIN_DEBUG
    printf("%s worker thread has started\n", srtp->t.tname);
#endif
    for (;;) {
        sched_yield();
        ssign = sin_wi_queue_get_item(srtp->control_queue, 0,  0);
        if (ssign != NULL && sin_signal_get_signum(ssign) == SIGTERM) {
            sin_signal_dtor(ssign);
            break;
        } else if (ssign != NULL) {
            sin_signal_dtor(ssign);
        }
    }
#ifdef SIN_DEBUG
    printf("%s worker thread has stopped\n", srtp->t.tname);
#endif
}

struct sin_rx_thread *
sin_rx_thread_ctor(struct sin_stance *sip, int *e)
{
    struct sin_rx_thread *srtp;
    int rval;

    srtp = malloc(sizeof(struct sin_rx_thread));
    if (srtp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(srtp, '\0', sizeof(struct sin_rx_thread));
    SIN_TYPE_SET(srtp, _SIN_TYPE_WRK_THREAD);
    srtp->t.tname = "rx_thread #0";
    srtp->sip = sip;
    srtp->control_queue = sin_wi_queue_ctor(1, e, "rx_thread control");
    if (srtp->control_queue == NULL) {
        free(srtp);
        return (NULL);
    }
    rval = pthread_create(&srtp->t.tid, NULL, (void *(*)(void *))&sin_rx_thread, srtp);
    if (rval != 0) {
        free(srtp);
        _SET_ERR(e, rval);
        return (NULL);
    }
    return (srtp);
}

void
sin_rx_thread_dtor(struct sin_rx_thread *srtp)
{
    struct sin_signal *ssign;

    SIN_TYPE_ASSERT(srtp, _SIN_TYPE_WRK_THREAD);
    ssign = sin_signal_ctor(SIGTERM, NULL);
    sin_wi_queue_put_item(ssign, srtp->control_queue);
    pthread_join(srtp->t.tid, NULL);
    sin_wi_queue_dtor(srtp->control_queue);
    free(srtp);
}
