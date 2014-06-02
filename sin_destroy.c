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
#include "sin_rx_thread.h"
#include "sin_tx_thread.h"
#include "sin_pkt_zone.h"
#include "sin_pkt_sorter.h"

void
sin_destroy(void *p)
{
    struct sin_stance *sip;

    sip = (struct sin_stance *)p;
    SIN_TYPE_ASSERT(sip, _SIN_TYPE_SINSTANCE);

    sin_rx_thread_dtor(sip->rx_hst_thread);
    sin_pkt_sorter_dtor(sip->rx_hst_sort);
    sin_rx_thread_dtor(sip->rx_phy_thread);
    sin_pkt_sorter_dtor(sip->rx_phy_sort);
    sin_tx_thread_dtor(sip->tx_hst_thread);
    sin_tx_thread_dtor(sip->tx_phy_thread);
    sin_pkt_zone_dtor(sip->rx_hst_zone);
    sin_pkt_zone_dtor(sip->tx_hst_zone);
    sin_pkt_zone_dtor(sip->rx_phy_zone);
    sin_pkt_zone_dtor(sip->tx_phy_zone);
    close(sip->netmap_fd);
    free(sip);
}
