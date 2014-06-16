#include <sys/types.h>
#include <sys/socket.h>
#ifdef SIN_DEBUG
#include <assert.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "include/libsinet.h"
#include "sin_type.h"
#include "sin_stance.h"
#include "sin_ringmon_thread.h"
#include "sin_rx_thread.h"
#include "sin_tx_thread.h"
#include "sin_pkt_zone.h"
#include "sin_pkt_sorter.h"

void
sin_destroy(void *p)
{
    struct sin_stance *sip;
    int i;

    sip = (struct sin_stance *)p;
    SIN_TYPE_ASSERT(sip, _SIN_TYPE_SINSTANCE);

    sin_ringmon_thread_dtor(sip->rx_mon_thread);
    sin_rx_thread_dtor(sip->hst.rx_thread);
    sin_pkt_sorter_dtor(sip->hst.rx_sort);
    for (i = 0; i < sip->nrings; i++) {
        sin_rx_thread_dtor(sip->phy[i].rx_thread);
        sin_pkt_sorter_dtor(sip->phy[i].rx_sort);
    }
    sin_tx_thread_dtor(sip->hst.tx_thread);
    for (i = 0; i < sip->nrings; i++) {
        sin_tx_thread_dtor(sip->phy[i].tx_thread);
    }
    sin_pkt_zone_dtor(sip->hst.rx_zone);
    sin_pkt_zone_dtor(sip->hst.tx_zone);
    for (i = 0; i < sip->nrings; i++) {
        sin_pkt_zone_dtor(sip->phy[i].rx_zone);
        sin_pkt_zone_dtor(sip->phy[i].tx_zone);
    }
    free(sip->phy);
    close(sip->netmap_fd);
    free(sip);
}
