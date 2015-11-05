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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "sin_debug.h"
#include "sin_types.h"
#include "sin_pkt.h"
#include "sin_ip4.h"
#include "sin_mem_fast.h"
#include "sin_list.h"
#include "sin_wi_queue.h"
#include "sin_sorter.h"

#include "udpm_sorter.h"

struct udphdr {
    uint16_t src_port ;              /* Source port */
    uint16_t dst_port;               /* Destination port */
    uint16_t length;                 /* Length */
    uint16_t checksum;               /* Checksum */
    uint16_t data[0];
} __attribute__((__packed__));

struct ip4_udp {
    struct ip iphdr;
    struct udphdr udphdr;
} __attribute__((__packed__));

struct ip4_udp_en10t {
    uint8_t ether_dhost[6];
    uint8_t ether_shost[6];
    uint16_t ether_type;
    struct ip4_udp ip4_udp;
} __attribute__((__packed__));

#define SIN_IP4_UDP_MINLEN	(14 + 20 + 8)

#define sin_ip_chksum_start(wsum) {(wsum) = 0; }
#define sin_ip_chksum_update(wsum, dp, len) { \
    const uint16_t *ww; \
    int nleft; \
    SIN_DEBUG_ASSERT((len % 2) == 0); \
    ww = (const uint16_t *)(dp); \
    for (nleft = (len); nleft > 1; nleft -= 2)  { \
        (wsum) += *ww++; \
    } \
}
#define sin_ip_chksum_update_data(wsum, dp, len) { \
    const uint16_t *ww; \
    int nleft; \
    union { \
        uint16_t us; \
        uint8_t uc[2]; \
    } last; \
    ww = (const uint16_t *)(dp); \
    for (nleft = (len); nleft > 1; nleft -= 2)  { \
        (wsum) += *ww++; \
    } \
    if (nleft == 1) { \
        last.uc[0] = *(const uint8_t *)ww; \
        last.uc[1] = 0; \
        (wsum) += last.us; \
    } \
}
#define sin_ip_chksum_fin(wsum, osum) { \
    (wsum) = ((wsum) >> 16) + ((wsum) & 0xffff); \
    (wsum) += ((wsum) >> 16); \
    osum = ~(wsum); \
}

#define UNUSED(x) (void)(x)

int
udpm_taste(struct sin_pkt *pkt, struct ps_arg *ap)
{
    struct ip4_udp_en10t *p;
    struct udphdr *udphdr;
    struct udpm_params *args;
    int dst_port, src_port;

    args = (struct udpm_params *)ap->ap;
    if (pkt->len < SIN_IP4_UDP_MINLEN) {
        return (0);
    }
    p = (struct ip4_udp_en10t *)pkt->buf;
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 1)
    printf("inspecting %p, ether_type = %hu, ip_v = %d, ip_p = %d, "
      "icmp_type %hhu\n", pkt, p->ether_type, p->ip4_icmp.iphdr.ip_v,
      p->ip4_icmp.iphdr.ip_p, p->ip4_icmp.icmphdr.icmp_type);
#endif
    if (p->ether_type != htons(0x0800)) {
        return (0);
    }
    if (p->ip4_udp.iphdr.ip_v != 0x4) {
        return (0);
    }
    if (p->ip4_udp.iphdr.ip_p != 0x11) {
        return (0);
    }
    udphdr = &(p->ip4_udp.udphdr);
    if (ntohs(udphdr->length) < offsetof(struct udphdr, data)) {
        return (0);
    }
    dst_port = ntohs(udphdr->dst_port);
    if (dst_port > args->port_max || dst_port < args->port_min) {
        return (0);
    }
    src_port = ntohs(udphdr->src_port);
    if (src_port > args->port_max || src_port < args->port_min) {
        return (0);
    }
    return (1);
}

static inline void
sin_ip4_udp_mirror(struct sin_pkt *pkt)
{
    struct ip4_udp_en10t *p;
    struct ip *iphdr;
    struct udphdr *udphdr;
#if defined(SIN_DEBUG)
    uint32_t wsum;
    static const union {
        uint16_t u16;
        uint8_t b8[2];
    } ppad = {.b8 = {0x0, 0x11}};
#endif

    p = (struct ip4_udp_en10t *)pkt->buf;
    sin_memswp(p->ether_shost, p->ether_dhost, 6);
    iphdr = &(p->ip4_udp.iphdr);
    sin_memswp((uint8_t *)&(iphdr->ip_src), (uint8_t *)&(iphdr->ip_dst),
      sizeof(struct in_addr));
#if defined(SIN_DEBUG)
    /*
     * Unless we actually mess with IP header fields, just swapping src/dst
     * is not affecting checksum.
     */
    iphdr->ip_sum = 0;
    iphdr->ip_sum = sin_ip4_cksum(iphdr, sizeof(struct ip));
#endif
    udphdr = &(p->ip4_udp.udphdr);
    sin_memswp((uint8_t *)&udphdr->src_port, (uint8_t *)&udphdr->dst_port,
      sizeof(uint16_t));
#if defined(SIN_DEBUG)
    /*
     * Unless we actually change data, just swapping src/dst ports
     * is not affecting checksum.
     */
    sin_ip_chksum_start(wsum);
    sin_ip_chksum_update(wsum, &(iphdr->ip_src), sizeof(iphdr->ip_src));
    sin_ip_chksum_update(wsum, &(iphdr->ip_dst), sizeof(iphdr->ip_dst));
    sin_ip_chksum_update(wsum, &(ppad.u16), sizeof(ppad.u16));
    sin_ip_chksum_update(wsum, &(udphdr->length), sizeof(udphdr->length));
    sin_ip_chksum_update(wsum, &(udphdr->src_port), sizeof(udphdr->src_port));
    sin_ip_chksum_update(wsum, &(udphdr->dst_port), sizeof(udphdr->dst_port));
    sin_ip_chksum_update(wsum, &(udphdr->length), sizeof(udphdr->length));
    sin_ip_chksum_update_data(wsum, &(udphdr->data), ntohs(udphdr->length) -
      sizeof(struct udphdr));
    sin_ip_chksum_fin(wsum, udphdr->checksum);
#endif
}

#if 0
void
sin_ip4_udp_debug(struct sin_pkt *pkt)
{
    struct ip4_udp_en10t *p;
    struct udphdr *udphdr;

    p = (struct ip4_udp_en10t *)pkt->buf;
    udphdr = &(p->ip4_udp.udphdr);
    printf("udp.src_port = %hu, udp.dst_port = %hu\n", ntohs(udphdr->src_port),
      ntohs(udphdr->dst_port));
}
#endif

void
udpm_proc(struct sin_list *pl, struct ps_arg *arg)
{
    struct sin_pkt *pkt;

    for (pkt = SIN_LIST_HEAD(pl); pkt != NULL; pkt = SIN_ITER_NEXT(pkt)) {
        sin_ip4_udp_mirror(pkt);
        SPKT_DBG_TRACE(pkt);
    }
    sin_wi_queue_put_items(pl, arg->outq);
}
