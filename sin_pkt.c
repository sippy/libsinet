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

#include <sys/types.h>
#include <net/netmap_user.h>
#ifdef SIN_DEBUG
#include <assert.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "sin_types.h"
#include "sin_errno.h"
#include "sin_pkt.h"

struct sin_pkt *
sin_pkt_ctor(struct sin_pkt_zone *my_zone, int zone_idx,
 struct netmap_ring *my_ring, int *e)
{
    struct sin_pkt *pkt;

    pkt = malloc(sizeof(struct sin_pkt));
    if (pkt == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(pkt, '\0', sizeof(struct sin_pkt));
    SIN_TYPE_SET(pkt, _SIN_TYPE_PKT);
    pkt->ts = malloc(sizeof(struct timeval));
    memset(pkt->ts, '\0', sizeof(struct timeval));
    pkt->my_zone = my_zone;
    pkt->my_ring = my_ring;
    pkt->zone_idx = zone_idx;
    pkt->my_slot = &(my_ring->slot[zone_idx]);
    pkt->buf = NETMAP_BUF(my_ring, pkt->my_slot->buf_idx);
    pthread_mutex_init(&pkt->mutex, NULL);

    return (pkt);
}


void
sin_pkt_dtor(struct sin_pkt *pkt)
{

    SIN_TYPE_ASSERT(pkt, _SIN_TYPE_PKT);
    pthread_mutex_destroy(&pkt->mutex);
    free(pkt->ts);
    free(pkt);
}

unsigned int
sin_pkt_setflags(struct sin_pkt *pkt, unsigned int sflags, unsigned int rflags)
{
    unsigned int oldflags;

    SIN_TYPE_ASSERT(pkt, _SIN_TYPE_PKT);
    pthread_mutex_lock(&pkt->mutex);
    oldflags = pkt->flags;
    pkt->flags |= sflags;
    pkt->flags &= ~rflags;
    pthread_mutex_unlock(&pkt->mutex);
    return (oldflags);
}
