#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "sin_type.h"
#include "sin_errno.h"
#include "sin_signal.h"
#include "sin_wi_queue.h"
#include "sin_wrk_thread.h"

static void
sin_wrk_thread_runner(struct sin_type_wrk_thread *swtp)
{

#ifdef SIN_DEBUG
    printf("%s has started\n", swtp->tname);
#endif
    swtp->runner(swtp);
#ifdef SIN_DEBUG
    printf("%s has stopped\n", swtp->tname);
#endif
}


int
sin_wrk_thread_ctor(struct sin_type_wrk_thread *swtp, const char *tname,
  void *(*start_routine)(void *), int *e)
{
    int rval;

    swtp->sin_type = _SIN_TYPE_WRK_THREAD;
    asprintf(&swtp->tname, "%s worker thread", tname);
    if (swtp->tname == NULL) {
        _SET_ERR(e, ENOMEM);
        return (-1);
    }
    swtp->ctrl_queue = sin_wi_queue_ctor(e, "%s control queue", tname);
    if (swtp->ctrl_queue == NULL) {
        goto er_undo_1;
    }
    swtp->sigterm = sin_signal_ctor(SIGTERM, e);
    if (swtp->sigterm == NULL) {
        goto er_undo_2;
    }
    swtp->runner = start_routine;
    rval = pthread_create(&swtp->tid, NULL, (void *(*)(void *))&sin_wrk_thread_runner, swtp);
    if (rval != 0) {
        _SET_ERR(e, rval);
        goto er_undo_3;
    }
    return (0);

er_undo_3:
    sin_signal_dtor(swtp->sigterm);
er_undo_2:
    sin_wi_queue_dtor(swtp->ctrl_queue);
er_undo_1:
    free(swtp->tname);
    return (-1);
}

int
sin_wrk_thread_check_ctrl(struct sin_type_wrk_thread *swtp)
{
    struct sin_signal *ssign;
    int signum;

    ssign = sin_wi_queue_get_item(swtp->ctrl_queue, 0,  0);
    if (ssign == NULL) {
        return (-1);
    }
    signum = sin_signal_get_signum(ssign);
    if (ssign != swtp->sigterm) {
        sin_signal_dtor(ssign);
    }
    return (signum);
}

void
sin_wrk_thread_dtor(struct sin_type_wrk_thread *swtp)
{

    sin_wi_queue_put_item(swtp->sigterm, swtp->ctrl_queue);
    pthread_join(swtp->tid, NULL);
    sin_signal_dtor(swtp->sigterm);
    /* Drain ctrl queue */
    while (sin_wrk_thread_check_ctrl(swtp) != -1) {
        continue;
    }
    sin_wi_queue_dtor(swtp->ctrl_queue);
    free(swtp->tname);
}
