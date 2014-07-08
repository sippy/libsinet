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

struct sin_type {
    unsigned int sin_type;
    char type_data[0];
};

struct sin_type_linkable {
    unsigned int sin_type;
    struct sin_type_linkable *sin_next;
    char type_data[0];
};

#define _SIN_TYPE_SINSTANCE     693750532
#define _SIN_TYPE_SOCKET        4061681943
#define _SIN_TYPE_ADDR          489855194
#define _SIN_TYPE_QUEUE         1319882625
#define _SIN_TYPE_EVENT         3336537370
#define _SIN_TYPE_PKT_ZONE      720778432
#define _SIN_TYPE_PKT           639956139
#define _SIN_TYPE_WI_QUEUE      1938993589
#define _SIN_TYPE_WRK_THREAD    1612654994
#define _SIN_TYPE_SIGNAL        229112560
#define _SIN_TYPE_PKT_SORTER	1943693179
#define _SIN_TYPE_ITERABLE      962174450

#ifdef SIN_DEBUG
# define SIN_TYPE_ASSERT(ssp, model_type) \
  assert((ssp)->t.sin_type == (model_type))
#else
# define SIN_TYPE_ASSERT(ssp, model_type) {}
#endif

#define SIN_TYPE_SET(ssp, type) {(ssp)->t.sin_type = (type);}

#define SIN_TYPE_LINK(cp, np) (cp)->t.sin_next = (struct sin_type_linkable *)(np)

#define SIN_TYPE_IS_LINKABLE(stp)  ((stp)->sin_type == _SIN_TYPE_PKT || \
  (stp)->sin_type == _SIN_TYPE_SIGNAL || (stp)->sin_type == _SIN_TYPE_ITERABLE)

#define SIN_ITER_NEXT(stlp)	((void *)((stlp)->t.sin_next))
