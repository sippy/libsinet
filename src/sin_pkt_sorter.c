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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct sin_pkt;

#include "sin_types.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_pkt.h"
#include "sin_list.h"
#include "sin_pkt_sorter.h"

struct ps_entry {
    struct sin_type_linkable t;
    int (*pkt_taste)(struct sin_pkt *);
    void (*pkt_consume)(struct sin_list *, void *);
    void *consume_arg;
    struct sin_list wrklist;
};

struct sin_pkt_sorter {
    struct sin_type t;
    struct ps_entry *first;
    void (*pkt_consume_default)(struct sin_list *, void *);
    void *consume_arg;
    struct sin_list wrklist;
};

struct sin_pkt_sorter *
sin_pkt_sorter_ctor(void (*consume_default)(struct sin_list *, void *),
  void *consume_arg, int *e)
{
    struct sin_pkt_sorter *inst;

    inst = malloc(sizeof(*inst));
    if (inst == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(inst, '\0', sizeof(*inst));
    SIN_TYPE_SET(inst, _SIN_TYPE_PKT_SORTER);
    inst->pkt_consume_default = consume_default;
    inst->consume_arg = consume_arg;
    return (inst);
}

int
sin_pkt_sorter_reg(struct sin_pkt_sorter *inst, int (*taste)(struct sin_pkt *),
  void (*consume)(struct sin_list *, void *), void *consume_arg, int *e)
{
    struct ps_entry *ent;

    ent = malloc(sizeof(*ent));
    if (ent == NULL) {
        _SET_ERR(e, ENOMEM);
        return (-1);
    }
    memset(ent, '\0', sizeof(*ent));
    SIN_TYPE_SET(inst, _SIN_TYPE_ITERABLE);
    ent->pkt_taste = taste;
    ent->pkt_consume = consume;
    ent->consume_arg = consume_arg;
    SIN_TYPE_LINK(ent, inst->first);
    inst->first = ent;
    return (0);
}

void
sin_pkt_sorter_proc(struct sin_pkt_sorter *spsp, struct sin_list *pl)
{
    struct ps_entry *psep;
    struct sin_pkt *pkt, *pkt_next;

    for (pkt = SIN_LIST_HEAD(pl); pkt != NULL; pkt = pkt_next) {
        SPKT_DBG_TRACE(pkt);
        for (psep = spsp->first; psep != NULL; psep = SIN_ITER_NEXT(psep)) {
            if (psep->pkt_taste(pkt)) {
                pkt_next = SIN_ITER_NEXT(pkt);
                SIN_TYPE_LINK(pkt, NULL);
                sin_list_append(&psep->wrklist, pkt);
                goto nextpkt;
            }
        }
        pkt_next = SIN_ITER_NEXT(pkt);
        SIN_TYPE_LINK(pkt, NULL);
        sin_list_append(&spsp->wrklist, pkt);
nextpkt:
        continue;
    }
    for (psep = spsp->first; psep != NULL; psep = SIN_ITER_NEXT(psep)) {
        if (!SIN_LIST_IS_EMPTY(&psep->wrklist)) {
            psep->pkt_consume(&psep->wrklist, psep->consume_arg);
            SIN_LIST_RESET(&psep->wrklist);
        }
    }
    if (!SIN_LIST_IS_EMPTY(&spsp->wrklist)) {
        spsp->pkt_consume_default(&spsp->wrklist, spsp->consume_arg);
        SIN_LIST_RESET(&spsp->wrklist);
    }
}

void
sin_pkt_sorter_dtor(struct sin_pkt_sorter *spsp)
{
    struct ps_entry *psep, *psep_next;

    for (psep = spsp->first; psep != NULL; psep = psep_next) {
        psep_next = SIN_ITER_NEXT(psep);
        free(psep);
    }
    free(spsp);
}
