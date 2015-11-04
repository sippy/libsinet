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

struct sin_pkt_zone;
struct netmap_ring;

#define SPKT_BUSY 0x1

struct sin_pkt
{
    struct sin_type_linkable t;
    struct sin_pkt_zone *my_zone;
    struct netmap_ring *my_ring;
    struct netmap_slot *my_slot;
    unsigned int zone_idx;
    struct timeval *ts;
    char *buf;
    unsigned int len;
    unsigned int flags;
    pthread_mutex_t mutex;
#if defined(SIN_DEBUG)
    char last_seen[256];
#endif
};

#if defined(SIN_DEBUG)
#include <stdio.h>

# define SPKT_DBG_TRACE(pp) { \
    pthread_mutex_lock(&((pp)->mutex)); \
    snprintf((pp)->last_seen, sizeof((pp)->last_seen), "%s(), %s:%d", \
      __func__, __FILE__, __LINE__); \
    pthread_mutex_unlock(&((pp)->mutex)); \
}
# define SPKT_DBG_TRACEF(pp, format, args...) { \
    pthread_mutex_lock(&((pp)->mutex)); \
    snprintf((pp)->last_seen, sizeof((pp)->last_seen), ("%s(), %s:%d: " format), \
      __func__, __FILE__, __LINE__, ## args); \
    pthread_mutex_unlock(&((pp)->mutex)); \
}
#else
# define SPKT_DBG_TRACE(pp)
# define SPKT_DBG_TRACEF(pp, format, args...)
#endif

struct sin_pkt *sin_pkt_ctor(struct sin_pkt_zone *my_zone,
  int zone_idx, struct netmap_ring *my_ring, int *sin_err);
void sin_pkt_dtor(struct sin_pkt *pkt);
unsigned int sin_pkt_setflags(struct sin_pkt *, unsigned int, unsigned int);

static inline int sin_pkt_isbusy(struct sin_pkt *pkt)
{
    int rval;

    pthread_mutex_lock(&pkt->mutex);
    rval = pkt->flags & SPKT_BUSY;
    pthread_mutex_unlock(&pkt->mutex);
    return (rval);
}
