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

static inline void
sin_pkt_zone_ret_pkt(struct sin_pkt *pkt)
{

    sin_pkt_zone_lock(pkt->my_zone);
#ifdef SIN_DEBUG
    assert(pkt->my_zone->first[pkt->zone_idx] == NULL);
#endif
    pkt->my_zone->first[pkt->zone_idx] = pkt;
    sin_pkt_zone_unlock(pkt->my_zone);
}

static inline void
sin_pkt_zone_swap(struct sin_pkt *p1, struct sin_pkt *p2)
{
    struct netmap_slot *src, *dst;

    src = p1->my_slot;
    dst = p2->my_slot;

    _SIN_SWAP_I(dst->buf_idx, src->buf_idx);
    _SIN_SWAP_P(p1->buf, p2->buf);
    dst->flags = NS_BUF_CHANGED;
    src->flags = NS_BUF_CHANGED;
    p2->len = dst->len = p1->len;
}

static inline void
sin_pkt_zone_copy(struct sin_pkt *p1, struct sin_pkt *p2)
{
    struct netmap_slot *dst;

    dst = p2->my_slot;

    sin_memcpy(p1->buf, p2->buf, p1->len);
    p2->len = dst->len = p1->len;
    dst->flags = NS_BUF_CHANGED;
}
