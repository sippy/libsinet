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

    tx_ring = sttp->sip->tx_ring;
    for (;;) {
        sched_yield();
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
    sttp->sip = sip;
    if (sin_wrk_thread_ctor(&sttp->t, "tx_thread #0",
      (void *(*)(void *))&sin_tx_thread, e) != 0) {
        free(sttp);
        return (NULL);
    }
    return (sttp);
}

void
sin_tx_thread_dtor(struct sin_tx_thread *sttp)
{

    SIN_TYPE_ASSERT(sttp, _SIN_TYPE_WRK_THREAD);
    sin_wrk_thread_dtor(&sttp->t);
    free(sttp);
}
