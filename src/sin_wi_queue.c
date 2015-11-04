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
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sin_debug.h"
#include "sin_types.h"
#include "sin_errno.h"
#include "sin_list.h"
#include "sin_wi_queue.h"

struct sin_wi_queue
{
    struct sin_type t;
    struct sin_type_linkable *head;
    struct sin_type_linkable *tail;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    unsigned int nwait;
    unsigned int length;
    char *name;
};

struct sin_wi_queue *
sin_wi_queue_ctor(int *e, const char *fmt, ...)
{
    struct sin_wi_queue *queue;
    va_list ap;
    int eval;

    queue = malloc(sizeof(*queue));
    if (queue == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(queue, '\0', sizeof(*queue));
    SIN_TYPE_SET(queue, _SIN_TYPE_WI_QUEUE);
    if ((eval = pthread_cond_init(&queue->cond, NULL)) != 0) {
        free(queue);
        _SET_ERR(e, eval);
        return (NULL);
    }
    eval = pthread_mutex_init(&queue->mutex, NULL);
    if (eval != 0) {
        pthread_cond_destroy(&queue->cond);
        free(queue);
        _SET_ERR(e, eval);
        return (NULL);
    }
    va_start(ap, fmt);
    vasprintf(&queue->name, fmt, ap);
    va_end(ap);
    if (queue->name == NULL) {
        pthread_cond_destroy(&queue->cond);
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    return (queue);
}

void
sin_wi_queue_dtor(struct sin_wi_queue *queue)
{

    SIN_TYPE_ASSERT(queue, _SIN_TYPE_WI_QUEUE);
    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->mutex);
    free(queue->name);
    free(queue);
}

int
sin_wi_queue_put_item(void *wi, struct sin_wi_queue *queue,
  unsigned int maxqlen)
{
    struct sin_type_linkable *stlp;

    stlp = (struct sin_type_linkable *)wi;

    pthread_mutex_lock(&queue->mutex);
    if (maxqlen > 0 && queue->length >= maxqlen) {
        if (queue->nwait > 0) {
            /* notify worker thread */
            pthread_cond_signal(&queue->cond);
        }
        pthread_mutex_unlock(&queue->mutex);
        return (-1);
    }

    stlp->sin_next = NULL;
    if (queue->head == NULL) {
        queue->head = stlp;
        queue->tail = stlp;
    } else {
        queue->tail->sin_next = stlp;
        queue->tail = stlp;
    }
    queue->length += 1;
#if 0
    if (queue->length > 99 && queue->length % 100 == 0)
        fprintf(stderr, "queue(%s): length %d\n", queue->name, queue->length);
#endif

    if (queue->nwait > 0) {
        /* notify worker thread */
        pthread_cond_signal(&queue->cond);
    }

    pthread_mutex_unlock(&queue->mutex);
    return (0);
}

void
sin_wi_queue_put_items(struct sin_list *lst, struct sin_wi_queue *queue)
{

#if defined(SIN_DEBUG) && (SIN_DEBUG_WAVE < 4)
    printf("sin_wi_queue_put_items(%s): adding %u items\n", queue->name,
      lst->len);
#endif

    pthread_mutex_lock(&queue->mutex);
    if (queue->head == NULL) {
        queue->head = lst->head;
        queue->tail = lst->tail;
        queue->length = lst->len;
    } else {
        queue->tail->sin_next = lst->head;
        queue->tail = lst->tail;
        queue->length += lst->len;
    }

    if (queue->nwait > 0) {
        /* notify worker thread */
        pthread_cond_signal(&queue->cond);
    }

    pthread_mutex_unlock(&queue->mutex);
}

int
sin_wi_queue_pump(struct sin_wi_queue *queue)
{
    int nwait;

    pthread_mutex_lock(&queue->mutex);
    nwait = queue->nwait;
    if (queue->nwait > 0) {
        /* notify worker thread */
        pthread_cond_signal(&queue->cond);
    }

    pthread_mutex_unlock(&queue->mutex);
    return (nwait);
}

void *
sin_wi_queue_get_item(struct sin_wi_queue *queue, int waitok,
  int return_on_wake)
{
    struct sin_type_linkable *wi;

    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        if (waitok == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return (NULL);
        }
        queue->nwait++;
        pthread_cond_wait(&queue->cond, &queue->mutex);
        queue->nwait--;
        if (queue->head == NULL && return_on_wake != 0) {
            pthread_mutex_unlock(&queue->mutex);
            return (NULL);
        }
    }
    wi = queue->head;
    queue->head = wi->sin_next;
    if (queue->head == NULL)
        queue->tail = NULL;
    queue->length -= 1;
    pthread_mutex_unlock(&queue->mutex);

    wi->sin_next = NULL;
    return (wi);
}

unsigned int
sin_wi_queue_get_items(struct sin_wi_queue *queue,  struct sin_list *lst,
  int waitok, int return_on_wake)
{
    unsigned int nadded;

    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        if (waitok == 0) {
            pthread_mutex_unlock(&queue->mutex);
            return (0);
        }
        queue->nwait++;
        pthread_cond_wait(&queue->cond, &queue->mutex);
        queue->nwait--;
        if (queue->head == NULL && return_on_wake != 0) {
            pthread_mutex_unlock(&queue->mutex);
            return (0);
        }
    }
    if (lst->head == NULL) {
        lst->head = queue->head;
        lst->tail = queue->tail;
        lst->len = nadded = queue->length;
    } else {
        lst->tail->sin_next = queue->head;
        lst->tail = queue->tail;
        lst->len += queue->length;
        nadded = queue->length;
    }
    queue->length = 0;
    queue->head = queue->tail = NULL;
    pthread_mutex_unlock(&queue->mutex);
    return (nadded);
}
