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
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

#include "include/libsinet.h"
#include "sin_types.h"
#include "sin_debug.h"
#include "sin_stance.h"
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
    close(sip->hst.queue_fd);
    for (i = 0; i < sip->nrings; i++) {
        sin_pkt_zone_dtor(sip->phy[i].rx_zone);
        sin_pkt_zone_dtor(sip->phy[i].tx_zone);
        close(sip->phy[i].queue_fd);
    }
    free(sip->phy);
    close(sip->netmap_fd);
    free(sip);
}
