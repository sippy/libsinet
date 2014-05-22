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

#include "include/libsinet.h"
#include "libsinet_internal.h"
#include "sin_errno.h"
#include "sin_stance.h"

void *
sin_init(const char *ifname, int *e)
{
    struct sin_stance *sip;
    struct nmreq req;

    sip = malloc(sizeof(struct sin_stance));
    if (sip == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(sip, '\0', sizeof(struct sin_stance));
    sip->sin_type = _SIN_TYPE_SINSTANCE;
    sip->netmap_fd = open("/dev/netmap", O_RDWR);
    if (sip->netmap_fd < 0) {
        _SET_ERR(e, errno);
        goto er_undo_0;
    }
    memset(&req, '\0', sizeof(req));
    strcpy(req.nr_name, ifname);
    req.nr_ringid = NETMAP_NO_TX_POLL;
    req.nr_version = NETMAP_API;
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
    sip->rx_ring = NETMAP_RXRING(sip->nifp, 0);
    sip->tx_ring = NETMAP_TXRING(sip->nifp, 0);
#ifdef SIN_DEBUG
    printf("number of tx slots: %d available slots: %d\n", sip->tx_ring->num_slots, sip->tx_ring->tail - sip->rx_ring->head);
    printf("number of rx slots: %d available slots: %d\n", sip->rx_ring->num_slots, sip->rx_ring->tail - sip->rx_ring->head);
#endif

    SIN_INCREF(sip);
    return (void *)sip;

er_undo_1:
    close(sip->netmap_fd);
er_undo_0:
    free(sip);
    return (NULL);
}
