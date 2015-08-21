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

struct sin_list {
    struct sin_type_linkable *head;
    struct sin_type_linkable *tail;
    unsigned int len;
};

static inline void
sin_list_append(struct sin_list *lst, void *p)
{
    struct sin_type_linkable *elem;

    elem = (struct sin_type_linkable *)p;
    SIN_DEBUG_ASSERT(SIN_TYPE_IS_LINKABLE(elem));
    SIN_DEBUG_ASSERT(elem->sin_next == NULL);
    if (lst->head == NULL) {
        lst->head = lst->tail = elem;
    } else {
        lst->tail->sin_next = elem;
        lst->tail = elem;
    }
    lst->len += 1;
}

#define SIN_LIST_RESET(lst) {(lst)->head = (lst)->tail = NULL; (lst)->len = 0;}
#define SIN_LIST_HEAD(lst)  (void *)((lst)->head)
#define SIN_LIST_IS_EMPTY(slp) ((slp)->len == 0)
