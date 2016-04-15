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

#if defined(__FreeBSD__)
#include <sys/thr.h>
#endif
#include <errno.h>
#include <pthread.h>
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sin_types.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_signal.h"
#include "sin_wi_queue.h"
#include "sin_wrk_thread.h"

struct sin_wrk_thread_private {
    pthread_t tid;
    char *tname;
    struct sin_wi_queue *ctrl_queue;
    struct sin_wi_queue *ctrl_notify_queue;
    struct sin_signal *sigterm;
    void *(*runner)(void *);
#if defined(__FreeBSD__)
    long lwpid;
#endif
};

static void sin_wrk_thread_dtor(struct sin_type_wrk_thread *swtp);
static int sin_wrk_thread_check_ctrl(struct sin_type_wrk_thread *swtp);
static void sin_wrk_thread_notify_on_ctrl(struct sin_type_wrk_thread *swtp,
  struct sin_wi_queue *ctrl_notify_queue);
static const char *sin_wrk_thread_get_tname(struct sin_type_wrk_thread *swtp);

static void
sin_wrk_thread_runner(struct sin_type_wrk_thread *swtp)
{

#if defined(__FreeBSD__)
    thr_self(&swtp->pvt->lwpid);
#endif

#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 5)
# if defined(__FreeBSD__)
    printf("%s has started, LWP = %ld\n", swtp->pvt->tname, swtp->pvt->lwpid);
# else
    printf("%s has started", swtp->pvt->tname);
# endif
#endif
    swtp->pvt->runner(swtp);
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 5)
    printf("%s has stopped\n", swtp->pvt->tname);
#endif
}

static const char *
sin_wrk_thread_get_tname(struct sin_type_wrk_thread *swtp)
{

    return (swtp->pvt->tname);
}

int
sin_wrk_thread_ctor(struct sin_type_wrk_thread *swtp, const char *tname,
  void *(*start_routine)(void *), int *e)
{
    int rval;

    swtp->sin_type = _SIN_TYPE_WRK_THREAD;
    swtp->pvt = malloc(sizeof(struct sin_wrk_thread_private));
    if (swtp->pvt == NULL) {
        _SET_ERR(e, ENOMEM);
        return (-1);
    }
    memset(swtp->pvt, '\0', sizeof(struct sin_wrk_thread_private));
    asprintf(&swtp->pvt->tname, "worker thread(%s)", tname);
    if (swtp->pvt->tname == NULL) {
        _SET_ERR(e, ENOMEM);
        goto er_undo_0;
    }
    swtp->pvt->ctrl_queue = sin_wi_queue_ctor(e, "%s control queue", tname);
    if (swtp->pvt->ctrl_queue == NULL) {
        goto er_undo_1;
    }
    swtp->pvt->sigterm = sin_signal_ctor(SIGTERM, e);
    if (swtp->pvt->sigterm == NULL) {
        goto er_undo_2;
    }
    swtp->pvt->runner = start_routine;
    rval = pthread_create(&swtp->pvt->tid, NULL, (void *(*)(void *))&sin_wrk_thread_runner, swtp);
    if (rval != 0) {
        _SET_ERR(e, rval);
        goto er_undo_3;
    }
#if defined(__FreeBSD__)
    pthread_set_name_np(swtp->pvt->tid, swtp->pvt->tname);
#endif
    swtp->dtor = &sin_wrk_thread_dtor;
    swtp->check_ctrl = &sin_wrk_thread_check_ctrl;
    swtp->notify_on_ctrl = &sin_wrk_thread_notify_on_ctrl;
    swtp->get_tname = &sin_wrk_thread_get_tname;
    return (0);

er_undo_3:
    sin_signal_dtor(swtp->pvt->sigterm);
er_undo_2:
    sin_wi_queue_dtor(swtp->pvt->ctrl_queue);
er_undo_1:
    free(swtp->pvt->tname);
er_undo_0:
    free(swtp->pvt);
    return (-1);
}

static int
sin_wrk_thread_check_ctrl(struct sin_type_wrk_thread *swtp)
{
    struct sin_signal *ssign;
    int signum;

    ssign = sin_wi_queue_get_item(swtp->pvt->ctrl_queue, 0,  0);
    if (ssign == NULL) {
        return (-1);
    }
    signum = sin_signal_get_signum(ssign);
    if (ssign != swtp->pvt->sigterm) {
        sin_signal_dtor(ssign);
    }
    return (signum);
}

static void
sin_wrk_thread_notify_on_ctrl(struct sin_type_wrk_thread *swtp,
  struct sin_wi_queue *ctrl_notify_queue)
{

    swtp->pvt->ctrl_notify_queue = ctrl_notify_queue;
}

static void
sin_wrk_thread_dtor(struct sin_type_wrk_thread *swtp)
{
    struct sin_wrk_thread_private *pp;

    pp = swtp->pvt;
    sin_wi_queue_put_item(pp->sigterm, pp->ctrl_queue, 1);
    if (pp->ctrl_notify_queue != NULL) {
        sin_wi_queue_pump(pp->ctrl_notify_queue);
    }
    pthread_join(pp->tid, NULL);
    sin_signal_dtor(pp->sigterm);
    /* Drain ctrl queue */
    while (sin_wrk_thread_check_ctrl(swtp) != -1) {
        continue;
    }
    sin_wi_queue_dtor(pp->ctrl_queue);
    free(pp->tname);
    free(pp);
}
