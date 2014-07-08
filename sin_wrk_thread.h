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

struct sin_type_wrk_thread;
struct sin_wi_queue;

typedef void (*sin_wrk_thread_dtor_t)(struct sin_type_wrk_thread *);
typedef int (*sin_wrk_thread_method_0_t)(struct sin_type_wrk_thread *);
typedef void (*sin_wrk_thread_notify_on_ctrl_t)(struct sin_type_wrk_thread *,
  struct sin_wi_queue *ctrl_notify_queue);
typedef const char * (*sin_wrk_thread_get_tname_t)(struct sin_type_wrk_thread *);

struct sin_wrk_thread_private;

struct sin_type_wrk_thread {
    unsigned int sin_type; /* Must be the first member */
    sin_wrk_thread_dtor_t dtor;
    sin_wrk_thread_method_0_t check_ctrl;
    sin_wrk_thread_notify_on_ctrl_t notify_on_ctrl;
    sin_wrk_thread_get_tname_t get_tname;
    struct sin_wrk_thread_private *pvt;
};

int sin_wrk_thread_ctor(struct sin_type_wrk_thread *swtp, const char *tname,
  void *(*start_routine)(void *), int *sin_err);
