#include <sys/ioctl.h>
#include <sys/mman.h>
#include <net/netmap_user.h>
#include <errno.h>
#include <fcntl.h>
#ifdef SIN_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sin_debug.h"
#include "sin_type.h"
#include "include/libsinet.h"
#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_stance.h"
#include "sin_pkt_zone.h"
#include "sin_rx_thread.h"
#include "sin_tx_thread.h"
#include "sin_pkt_sorter.h"
#include "sin_ip4_icmp.h"

void *
sin_init(const char *ifname, int *e)
{
    struct sin_stance *sip;
    struct nmreq req;
    struct sin_wi_queue *tx_phy_queue;
    void *sarg;

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
    sip->rx_phy_ring = NETMAP_RXRING(sip->nifp, 0);
    sip->tx_phy_ring = NETMAP_TXRING(sip->nifp, 0);
#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 3)
    printf("number of tx rings: %d\n", req.nr_tx_rings);
    printf("number of rx rings: %d\n", req.nr_rx_rings);
    printf("number of tx slots: %d available slots: %d\n",
      sip->tx_phy_ring->num_slots, sip->tx_phy_ring->tail -
      sip->tx_phy_ring->head);
    printf("number of rx slots: %d available slots: %d\n",
      sip->rx_phy_ring->num_slots, sip->rx_phy_ring->tail -
      sip->rx_phy_ring->head);
#endif
    sip->tx_phy_free = sin_pkt_zone_ctor(sip->tx_phy_ring, sip->netmap_fd, e);
    if (sip->tx_phy_free == NULL) {
        goto er_undo_1;
    }
    sip->rx_phy_free = sin_pkt_zone_ctor(sip->rx_phy_ring, sip->netmap_fd, e);
    if (sip->rx_phy_free == NULL) {
        goto er_undo_2;
    }
    sip->tx_phy_thread = sin_tx_thread_ctor(sip->tx_phy_ring, sip->tx_phy_free,
      e);
    if (sip->tx_phy_thread == NULL) {
        goto er_undo_3;
    }
#ifdef SIN_DEBUG
    sarg = sip->rx_phy_free;
#else
    sarg = NULL;
#endif
    sip->rx_phy_sort = sin_pkt_sorter_ctor(sin_pkt_zone_ret_all, sarg, e);
    if (sip->rx_phy_sort == NULL) {
        goto er_undo_4;
    }
    tx_phy_queue = sin_tx_thread_get_out_queue(sip->tx_phy_thread);
    if (sin_pkt_sorter_reg(sip->rx_phy_sort, sin_ip4_icmp_taste,
      sin_ip4_icmp_proc, tx_phy_queue, e) != 0) {
        goto er_undo_5;
    }
    sip->rx_phy_thread = sin_rx_thread_ctor(sip->rx_phy_ring, sip->rx_phy_free,
      sip->rx_phy_sort, e);
    if (sip->rx_phy_thread == NULL) {
        goto er_undo_5;
    }

    SIN_INCREF(sip);
    return (void *)sip;

#if 0
er_undo_6:
    sin_rx_thread_dtor(sip->rx_phy_thread);
#endif
er_undo_5:
    sin_pkt_sorter_dtor(sip->rx_phy_sort);
er_undo_4:
    sin_tx_thread_dtor(sip->tx_phy_thread);
er_undo_3:
    sin_pkt_zone_dtor(sip->rx_phy_free);
er_undo_2:
    sin_pkt_zone_dtor(sip->tx_phy_free);
er_undo_1:
    close(sip->netmap_fd);
er_undo_0:
    free(sip);
    return (NULL);
}
