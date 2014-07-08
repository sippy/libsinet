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

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <net/netmap_user.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sin_debug.h"
#include "sin_types.h"
#include "include/libsinet.h"
#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_stance.h"
#include "sin_pkt_zone.h"
#include "sin_ringmon_thread.h"
#include "sin_rx_thread.h"
#include "sin_tx_thread.h"
#include "sin_pkt_sorter.h"
#include "sin_ip4_icmp.h"
#include "sin_wi_queue.h"

void *
sin_init(const char *ifname, int *e)
{
    struct sin_stance *sip;
    struct nmreq req;
    struct sin_wi_queue *tx_phy_queue, *tx_hst_queue;
    int i;

    sip = malloc(sizeof(struct sin_stance));
    if (sip == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(sip, '\0', sizeof(struct sin_stance));
    SIN_TYPE_SET(sip, _SIN_TYPE_SINSTANCE);
    sip->netmap_fd = open("/dev/netmap", O_RDWR);
    if (sip->netmap_fd < 0) {
        _SET_ERR(e, errno);
        goto er_undo_0;
    }
    memset(&req, '\0', sizeof(req));
    strcpy(req.nr_name, ifname);
    req.nr_version = NETMAP_API;
    req.nr_flags = NR_REG_NIC_SW;
#if 0
    req.nr_ringid |= NETMAP_NO_TX_POLL;
#endif
    if (ioctl(sip->netmap_fd, NIOCREGIF, &req) < 0) {
        _SET_ERR(e, errno);
        goto er_undo_1;
    }
    sip->mem = mmap(0, req.nr_memsize, PROT_WRITE | PROT_READ, MAP_SHARED,
      sip->netmap_fd, 0);
    if (sip->mem == MAP_FAILED) {
        _SET_ERR(e, errno);
        goto er_undo_1;
    }
    sip->nifp = NETMAP_IF(sip->mem, req.nr_offset);
    sip->nrings = req.nr_rx_rings;
    sip->phy = malloc(sizeof(struct wrk_set) * sip->nrings);
    if (sip->phy == NULL) {
        _SET_ERR(e, ENOMEM);
        goto er_undo_1;
    }
    memset(sip->phy, '\0', sizeof(struct wrk_set) * sip->nrings);
    for (i = 0; i < sip->nrings; i++) {
        sip->phy[i].rx_ring = NETMAP_RXRING(sip->nifp, i);
        sip->phy[i].tx_ring = NETMAP_TXRING(sip->nifp, i);
    }
    sip->hst.rx_ring = NETMAP_RXRING(sip->nifp, i);
    sip->hst.tx_ring = NETMAP_TXRING(sip->nifp, i);
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 6)
    printf("number of tx rings: %d\n", req.nr_tx_rings);
    printf("number of rx rings: %d\n", req.nr_rx_rings);
    printf(" number of tx slots(hw): %d available slots: %d\n",
      sip->phy[0].tx_ring->num_slots, sip->phy[0].tx_ring->tail -
      sip->phy[0].tx_ring->head);
    printf(" number of rx slots(hw): %d available slots: %d\n",
      sip->phy[0].rx_ring->num_slots, sip->phy[0].rx_ring->tail -
      sip->phy[0].rx_ring->head);
    printf(" number of tx slots(sw): %d available slots: %d\n",
      sip->hst.tx_ring->num_slots, sip->hst.tx_ring->tail -
      sip->hst.tx_ring->head);
    printf(" number of rx slots(sw): %d available slots: %d\n",
      sip->hst.rx_ring->num_slots, sip->hst.rx_ring->tail -
      sip->hst.rx_ring->head);
#if 0
    _SET_ERR(e, EDOOFUS);
    goto er_undo_1;
#endif
#endif
    for (i = 0; i < sip->nrings; i++) {
        sip->phy[i].tx_zone = sin_pkt_zone_ctor(sip->phy[i].tx_ring,
          sip->netmap_fd, e);
        if (sip->phy[i].tx_zone == NULL) {
            goto er_undo_2;
        }
    }
    for (i = 0; i < sip->nrings; i++) {
        sip->phy[i].rx_zone = sin_pkt_zone_ctor(sip->phy[i].rx_ring,
          sip->netmap_fd, e);
        if (sip->phy[i].rx_zone == NULL) {
            goto er_undo_3;
        }
    }
    sip->hst.tx_zone = sin_pkt_zone_ctor(sip->hst.tx_ring, sip->netmap_fd, e);
    if (sip->hst.tx_zone == NULL) {
        goto er_undo_3;
    }
    sip->hst.rx_zone = sin_pkt_zone_ctor(sip->hst.rx_ring, sip->netmap_fd, e);
    if (sip->hst.rx_zone == NULL) {
        goto er_undo_4;
    }

    for (i = 0; i < sip->nrings; i++) {
        char tname[64];

        sprintf(tname, "tx_phy #%d", i);
        sip->phy[i].tx_thread = sin_tx_thread_ctor(tname,
          sip->phy[i].tx_ring, sip->phy[i].tx_zone, e);
        if (sip->phy[i].tx_thread == NULL) {
            goto er_undo_6;
        }
    }
    sip->hst.tx_thread = sin_tx_thread_ctor("tx_hst #0", sip->hst.tx_ring,
      sip->hst.tx_zone, e);
    if (sip->hst.tx_thread == NULL) {
        goto er_undo_6;
    }

    tx_hst_queue = sin_tx_thread_get_out_queue(sip->hst.tx_thread);
    for (i = 0; i < sip->nrings; i++) {
        sip->phy[i].rx_sort = sin_pkt_sorter_ctor(
          (void *)sin_wi_queue_put_items, tx_hst_queue, e);
        if (sip->phy[i].rx_sort == NULL) {
            goto er_undo_8;
        }
        tx_phy_queue = sin_tx_thread_get_out_queue(sip->phy[i].tx_thread);
        if (sin_pkt_sorter_reg(sip->phy[i].rx_sort, sin_ip4_icmp_taste,
          sin_ip4_icmp_proc, tx_phy_queue, e) != 0) {
            goto er_undo_8;
        }
    }

    for (i = 0; i < sip->nrings; i++) {
        char tname[64];

        sprintf(tname, "rx_phy #%d", i);
        sip->phy[i].rx_thread = sin_rx_thread_ctor(tname, sip->phy[i].rx_ring,
          sip->phy[i].rx_zone, sip->phy[i].rx_sort, e);
        if (sip->phy[i].rx_thread == NULL) {
            goto er_undo_9;
        }
    }

    tx_phy_queue = sin_tx_thread_get_out_queue(sip->phy[0].tx_thread);
    sip->hst.rx_sort = sin_pkt_sorter_ctor((void *)sin_wi_queue_put_items,
      tx_phy_queue, e);
    if (sip->hst.rx_sort == NULL) {
        goto er_undo_9;
    }
    sip->hst.rx_thread = sin_rx_thread_ctor("rx_hst #0", sip->hst.rx_ring,
      sip->hst.rx_zone, sip->hst.rx_sort, e);
    if (sip->hst.rx_thread == NULL) {
        goto er_undo_10;
    }

    sip->rx_mon_thread = sin_ringmon_thread_ctor("rx_mon", sip->netmap_fd,
      e);
    if (sip->rx_mon_thread == NULL) {
        goto er_undo_11;
    }
    for (i = 0; i < sip->nrings; i++) {
        if (sin_ringmon_register(sip->rx_mon_thread, sip->phy[i].rx_ring,
          (void (*)(void *))&sin_rx_thread_wakeup, sip->phy[i].rx_thread, e) < 0) {
            goto er_undo_12;
        }
    }
    if (sin_ringmon_register(sip->rx_mon_thread, sip->hst.rx_ring,
      (void (*)(void *))&sin_rx_thread_wakeup, sip->hst.rx_thread, e) < 0) {
        goto er_undo_12;
    }

    SIN_INCREF(sip);
    return (void *)sip;

er_undo_12:
    sin_ringmon_thread_dtor(sip->rx_mon_thread);
er_undo_11:
    sin_rx_thread_dtor(sip->hst.rx_thread);
er_undo_10:
    sin_pkt_sorter_dtor(sip->hst.rx_sort);
er_undo_9:
    for (i = 0; i < sip->nrings; i++) {
        if (sip->phy[i].rx_thread != NULL) {
            sin_rx_thread_dtor(sip->phy[i].rx_thread);
        }
    }
er_undo_8:
    for (i = 0; i < sip->nrings; i++) {
        if (sip->phy[i].rx_sort != NULL) {
            sin_pkt_sorter_dtor(sip->phy[i].rx_sort);
        }
    }
#if 0
er_undo_7:
#endif
    sin_tx_thread_dtor(sip->hst.tx_thread);
er_undo_6:
    for (i = 0; i < sip->nrings; i++) {
        if (sip->phy[i].tx_thread != NULL) {
            sin_tx_thread_dtor(sip->phy[i].tx_thread);
        }
    }
#if 0
er_undo_5:
#endif
    sin_pkt_zone_dtor(sip->hst.rx_zone);
er_undo_4:
    sin_pkt_zone_dtor(sip->hst.tx_zone);
er_undo_3:
    for (i = 0; i < sip->nrings; i++) {
        if (sip->phy[i].rx_zone != NULL) {
            sin_pkt_zone_dtor(sip->phy[i].rx_zone);
        }
    }
er_undo_2:
    for (i = 0; i < sip->nrings; i++) {
        if (sip->phy[i].tx_zone != NULL) {
            sin_pkt_zone_dtor(sip->phy[i].tx_zone);
        }
    }
    free(sip->phy);
er_undo_1:
    close(sip->netmap_fd);
er_undo_0:
    free(sip);
    return (NULL);
}
