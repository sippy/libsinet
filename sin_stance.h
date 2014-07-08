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

struct sin_pkt;
struct sin_pkt_zone;
struct sin_rx_thread;
struct sin_tx_thread;
struct sin_pkt_sorter;
struct sin_ringmon_thread;

struct wrk_set {
    struct netmap_ring *tx_ring;
    struct netmap_ring *rx_ring;
    struct sin_pkt_zone *rx_zone;
    struct sin_pkt_zone *tx_zone;
    struct sin_tx_thread *tx_thread;
    struct sin_rx_thread *rx_thread;
    struct sin_pkt_sorter *rx_sort;
};

struct sin_stance {
    struct sin_type t;
    unsigned int sin_nref;
    int netmap_fd;
    int nrings;
    void *mem;
    struct netmap_if *nifp;
    struct wrk_set *phy;
    struct wrk_set hst;
    struct sin_ringmon_thread *rx_mon_thread;
};
