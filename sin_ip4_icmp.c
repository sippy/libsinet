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
#include <netinet/in.h>
#include <netinet/ip.h>
#ifdef SIN_DEBUG
#include <assert.h>
#endif
#include <stdio.h>
#include <string.h>

#include "sin_debug.h"
#include "sin_types.h"
#include "sin_pkt.h"
#include "sin_ip4.h"
#include "sin_ip4_icmp.h"
#include "sin_mem_fast.h"
#include "sin_list.h"
#include "sin_wi_queue.h"

struct icmphdr {
    u_char  icmp_type;              /* type of message, see below */
    u_char  icmp_code;              /* type sub code */
    u_short icmp_cksum;             /* ones complement cksum of struct */
    u_short icmp_id;
    u_short icmp_seq;
} __attribute__((__packed__));

struct ip4_icmp {
    struct ip iphdr;
    struct icmphdr icmphdr;
} __attribute__((__packed__));

struct ip4_icmp_en10t {
    uint8_t ether_dhost[6];
    uint8_t ether_shost[6];
    uint16_t ether_type;
    struct ip4_icmp ip4_icmp;
} __attribute__((__packed__));

#define SIN_IP4_ICMP_MINLEN	(14 + 20 + 16)

int
sin_ip4_icmp_taste(struct sin_pkt *pkt)
{
    struct ip4_icmp_en10t *p;

    if (pkt->len < SIN_IP4_ICMP_MINLEN) {
        return (0);
    }
    p = (struct ip4_icmp_en10t *)pkt->buf;
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 1)
    printf("inspecting %p, ether_type = %hu, ip_v = %d, ip_p = %d, "
      "icmp_type %hhu\n", pkt, p->ether_type, p->ip4_icmp.iphdr.ip_v,
      p->ip4_icmp.iphdr.ip_p, p->ip4_icmp.icmphdr.icmp_type);
#endif
    if (p->ether_type != htons(0x0800)) {
        return (0);
    }
    if (p->ip4_icmp.iphdr.ip_v != 0x4) {
        return (0);
    }
    if (p->ip4_icmp.iphdr.ip_p != 0x1) {
        return (0);
    }
    if (p->ip4_icmp.icmphdr.icmp_type != 0x8) {
        return (0);
    }
    return (1);
}

int
sin_ip4_icmp_repl_taste(struct sin_pkt *pkt)
{
    struct ip4_icmp_en10t *p;

    if (pkt->len < SIN_IP4_ICMP_MINLEN) {
        return (0);
    }
    p = (struct ip4_icmp_en10t *)pkt->buf;
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 1)
    printf("inspecting %p, ether_type = %hu, ip_v = %d, ip_p = %d, "
      "icmp_type %hhu\n", pkt, p->ether_type, p->ip4_icmp.iphdr.ip_v,
      p->ip4_icmp.iphdr.ip_p, p->ip4_icmp.icmphdr.icmp_type);
#endif
    if (p->ether_type != htons(0x0800)) {
        return (0);
    }
    if (p->ip4_icmp.iphdr.ip_v != 0x4) {
        return (0);
    }
    if (p->ip4_icmp.iphdr.ip_p != 0x1) {
        return (0);
    }
    if (p->ip4_icmp.icmphdr.icmp_type != 0x0) {
        return (0);
    }
    return (1);
}

void
sin_ip4_icmp_req2rpl(struct sin_pkt *pkt)
{
    struct ip4_icmp_en10t *p;
    struct ip *iphdr;
    struct icmphdr *icmphdr;

    p = (struct ip4_icmp_en10t *)pkt->buf;
    sin_memswp(p->ether_shost, p->ether_dhost, 6);
    iphdr = &(p->ip4_icmp.iphdr);
    sin_memswp((uint8_t *)&(iphdr->ip_src), (uint8_t *)&(iphdr->ip_dst),
      sizeof(struct in_addr));
    iphdr->ip_sum = 0;
    iphdr->ip_sum = sin_ip4_cksum(iphdr, sizeof(struct ip));
    icmphdr = &(p->ip4_icmp.icmphdr);
    icmphdr->icmp_type = 0x0;
    icmphdr->icmp_cksum += htons(0x0800);
}

void
sin_ip4_icmp_debug(struct sin_pkt *pkt)
{
    struct ip4_icmp_en10t *p;
    struct icmphdr *icmphdr;

    p = (struct ip4_icmp_en10t *)pkt->buf;
    icmphdr = &(p->ip4_icmp.icmphdr);
    printf("icmp.id = %hu, icmp.seq = %hu\n", ntohs(icmphdr->icmp_id),
      ntohs(icmphdr->icmp_seq));
}

void
sin_ip4_icmp_proc(struct sin_list *pl, void *arg)
{
    struct sin_wi_queue *outq;
    struct sin_pkt *pkt;

    outq = (struct sin_wi_queue *)arg;
    for (pkt = SIN_LIST_HEAD(pl); pkt != NULL; pkt = SIN_ITER_NEXT(pkt)) {
        sin_ip4_icmp_req2rpl(pkt);
    }
    sin_wi_queue_put_items(pl, outq);
}
