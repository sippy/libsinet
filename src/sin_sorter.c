/*
 * Copyright (c) 2015 Sippy Software, Inc., http://www.sippysoft.com
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

#include "sin_types.h"
#include "sin_stance.h"
#include "sin_pkt_sorter.h"
#include "sin_sorter.h"
#include "sin_tx_thread.h"
#include "sin_wi_queue.h"

int
sin_sorter_reg(void *s, int (*taste)(struct sin_pkt *, struct ps_arg *),
  void (*consume)(struct sin_list *, struct ps_arg *), void *ap, int *e)
{
    struct sin_stance *sip;
    struct sin_wi_queue *tx_phy_queue;
    int i;

    sip = (struct sin_stance *)s;
    for (i = 0; i < sip->nrings; i++) {
        tx_phy_queue = sin_tx_thread_get_out_queue(sip->phy[i].tx_thread);
        if (sin_pkt_sorter_reg(sip->phy[i].rx_sort, tx_phy_queue,
          (sin_psort_taste_f)taste, (sin_psort_consume_f)consume, ap, e) != 0) {
            return (-1);
        }
    }
    return (0);
}
