#include <net/netmap_user.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "sin_errno.h"
#include "sin_stance.h"
#include "sin_wrk_thread.h"
#include "sin_rx_thread.h"

struct sin_wi_queue;

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

    rx_ring = srtp->sip->rx_ring;
    for (;;) {
        sched_yield();
    }
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
    srtp->sip = sip;
    rval = pthread_create(&srtp->t.tid, NULL, (void *(*)(void *))&sin_rx_thread, srtp);
    if (rval != 0) {
        free(srtp);
        _SET_ERR(e, rval);
        return (NULL);
    }
    return (srtp);
}
