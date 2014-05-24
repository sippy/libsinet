#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include "include/libsinet.h"
#include "sin_type.h"
#include "sin_stance.h"
#include "sin_rx_thread.h"
#include "sin_tx_thread.h"
#include "sin_pkt_zone.h"

void
sin_destroy(void *p)
{
    struct sin_stance *sip;

    sip = (struct sin_stance *)p;
    SIN_TYPE_ASSERT(sip, _SIN_TYPE_SINSTANCE);

    sin_rx_thread_dtor(sip->rx_thread);
    sin_tx_thread_dtor(sip->tx_thread);
    sin_pkt_zone_dtor(sip->rx_free);
    sin_pkt_zone_dtor(sip->tx_free);
    close(sip->netmap_fd);
    free(sip);
}
