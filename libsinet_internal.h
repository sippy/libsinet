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

#ifndef _LIBSINET_INTERNAL_H_
#define _LIBSINET_INTERNAL_H_

#define	_SIN_TYPE_SOCKET	4061681943
#define	_SIN_TYPE_ADDR		489855194
#define	_SIN_TYPE_QUEUE		4294967296
#define	_SIN_TYPE_EVENT		3336537370

struct sin_addr {
    unsigned int sin_type;
    struct sockaddr *addr;
    socklen_t addrlen;
};

struct sin_socket {
    unsigned int sin_type;
    struct sin_addr *dst;
    struct sin_addr *src;
    int last_errno;
};

struct sin_queue {
    unsigned int sin_type;
};

#define	SIN_TYPE_ASSERT(sin_struct, model_type)	\
  assert((sin_struct)->sin_type == (model_type))

#endif /* _LIBSINET_INTERNAL_H_ */
