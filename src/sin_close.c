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

#include "include/libsinet.h"
#include "sin_types.h"
#include "sin_debug.h"
#include "libsinet_internal.h"

int
sin_close(void *s)
{
    struct sin_socket *ssp;

    ssp = (struct sin_socket *)s;
    SIN_TYPE_ASSERT(ssp, _SIN_TYPE_SOCKET);

    if (ssp->src != NULL) {
        SIN_TYPE_ASSERT(ssp->src, _SIN_TYPE_ADDR);
        ssp->src->t.sin_type = ~ssp->src->t.sin_type;
        free(ssp->src);
    }
    if (ssp->dst != NULL) {
        SIN_TYPE_ASSERT(ssp->dst, _SIN_TYPE_ADDR);
        ssp->dst->t.sin_type = ~ssp->dst->t.sin_type;
        free(ssp->dst);
    }

    ssp->t.sin_type = ~ssp->t.sin_type;
    free(ssp);

    return (0);
}
