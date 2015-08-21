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
#include <stdlib.h>
#include <string.h>

#include "sin_types.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_signal.h"

struct sin_signal
{
    struct sin_type_linkable t;
    int signum;
};

struct sin_signal *
sin_signal_ctor(int signum, int *e)
{
    struct sin_signal *ssign;

    ssign = malloc(sizeof(struct sin_signal));
    if (ssign == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(ssign, '\0', sizeof(struct sin_signal));
    SIN_TYPE_SET(ssign, _SIN_TYPE_SIGNAL);
    ssign->signum = signum;

    return (ssign);
}


void
sin_signal_dtor(struct sin_signal *ssign)
{

    SIN_TYPE_ASSERT(ssign, _SIN_TYPE_SIGNAL);
    free(ssign);
}

int
sin_signal_get_signum(struct sin_signal *ssign)
{

    SIN_TYPE_ASSERT(ssign, _SIN_TYPE_SIGNAL);
    return (ssign->signum);
}
