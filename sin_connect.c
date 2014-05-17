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
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "libsinet.h"
#include "libsinet_internal.h"
#include "sin_errno.h"

int
sin_connect(void *s, const struct sockaddr *name, socklen_t namelen)
{
    struct sin_socket *ssp;
    struct sin_addr *daddr;

    ssp = (struct sin_socket *)s;
    if (ssp->dst == NULL) {
        ssp->dst = malloc(sizeof(struct sin_addr) + namelen);
        if (ssp->dst == NULL) {
            _sin_set_errno(ssp, ENOMEM);
            return (-1);
        }
        ssp->dst->sin_type = _SIN_TYPE_ADDR;
        ssp->dst->addr = (struct sockaddr *)((char *)ssp->dst +
          sizeof(struct sin_addr));
    } else if (namelen > ssp->dst->addrlen) {
        SIN_TYPE_ASSERT(ssp->dst, _SIN_TYPE_ADDR);
        daddr = realloc(ssp->dst, sizeof(struct sin_addr) + namelen);
        if (daddr == NULL) {
            _sin_set_errno(ssp, ENOMEM);
            return (-1);
        }
        ssp->dst = daddr;
    }
    memcpy(ssp->dst->addr, name, namelen);
    ssp->dst->addrlen = namelen;

    return (0);
}
