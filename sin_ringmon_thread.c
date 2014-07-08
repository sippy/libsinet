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
#include <poll.h>
#include <sched.h>
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
#include "sin_wrk_thread.h"
#include "sin_ringmon_thread.h"

struct ringmon_client {
    struct sin_type_linkable t;
    struct netmap_ring *rx_ring;
    void (*client_wakeup)(void *);
    void *wakeup_arg;
};    

struct sin_ringmon_thread
{
    struct sin_type_wrk_thread t;
    int netmap_fd;
    struct ringmon_client *first;
};

#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 5)
static void
ring_debug(const char *tname, struct netmap_ring *ring)
{
     printf("%s: debug_ring: ring->head = %u, ring->cur = %u, "
       "ring->tail = %u\n", tname, ring->head, ring->cur, ring->tail);
}
#else
#define	ring_debug(a, b)	{}
#endif

static void
sin_ringmon_thread(struct sin_ringmon_thread *srmtp)
{
    const char *tname;
    struct pollfd fds;
    struct ringmon_client *rmcp;
    int nready, yield;

    tname = sin_wrk_thread_get_tname(&srmtp->t);
    fds.fd = srmtp->netmap_fd;
    fds.events = POLLIN;
    yield = 0;
    for (;;) {
        if (sin_wrk_thread_check_ctrl(&srmtp->t) == SIGTERM) {
            break;
        }
        nready = poll(&fds, 1, 10);
        if (nready <= 0 || (fds.revents & POLLIN) == 0 ||
          srmtp->first == NULL) {
            continue;
        }
        for (rmcp = srmtp->first; rmcp != NULL; rmcp =  SIN_ITER_NEXT(rmcp)) {
            if (nm_ring_empty(rmcp->rx_ring)) {
                ring_debug(tname, rmcp->rx_ring);
                continue;
            }
            rmcp->client_wakeup(rmcp->wakeup_arg);
            yield = 1;
        }
        if (yield != 0) {
            sched_yield();
            yield = 0;
        }
    }
}

struct sin_ringmon_thread *
sin_ringmon_thread_ctor(const char *tname, int netmap_fd,
  int *e)
{
    struct sin_ringmon_thread *srmtp;

    srmtp = malloc(sizeof(struct sin_ringmon_thread));
    if (srmtp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(srmtp, '\0', sizeof(struct sin_ringmon_thread));
    srmtp->netmap_fd = netmap_fd;

    if (sin_wrk_thread_ctor(&srmtp->t, tname,
      (void *(*)(void *))&sin_ringmon_thread, e) != 0) {
        free(srmtp);
        return (NULL);
    }
    return (srmtp);
}

int
sin_ringmon_register(struct sin_ringmon_thread *srmtp, struct netmap_ring *rx_ring,
  void (*client_wakeup)(void *), void *wakeup_arg, int *e)
{
    struct ringmon_client *rmcp;

    rmcp = malloc(sizeof(struct ringmon_client));
    if (rmcp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (-1);
    }
    memset(rmcp, '\0', sizeof(struct ringmon_client));
    SIN_TYPE_SET(rmcp, _SIN_TYPE_ITERABLE);
    rmcp->rx_ring = rx_ring;
    rmcp->client_wakeup = client_wakeup;
    rmcp->wakeup_arg = wakeup_arg;
    SIN_TYPE_LINK(rmcp, srmtp->first);
    srmtp->first = rmcp;
    return (0);
}

void
sin_ringmon_thread_dtor(struct sin_ringmon_thread *srmtp)
{
    struct ringmon_client *rmcp, *rmcp_next;

    SIN_TYPE_ASSERT(srmtp, _SIN_TYPE_WRK_THREAD);
    sin_wrk_thread_dtor(&srmtp->t);
    for (rmcp = srmtp->first; rmcp != NULL; rmcp = rmcp_next) {
        rmcp_next = SIN_ITER_NEXT(rmcp);
        free(rmcp);
    }
    free(srmtp);
}
