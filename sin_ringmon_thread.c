#include <net/netmap_user.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#ifdef SIN_DEBUG
#include <assert.h>
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "sin_type.h"
#include "sin_debug.h"
#include "sin_errno.h"
#include "sin_wrk_thread.h"
#include "sin_ringmon_thread.h"

struct ringmon_client {
    struct sin_type_linkable t;
    struct netmap_ring *rx_ring;
    void (*client_wakeup)(void *);
    void *wakeup_arg;
};    

struct sin_ringmon_thread
{
    struct sin_type_wrk_thread t;
    int netmap_fd;
    struct ringmon_client *first;
};

static void
sin_ringmon_thread(struct sin_ringmon_thread *srmtp)
{
    const char *tname;
    struct pollfd fds;
    struct ringmon_client *rmcp;
    int nready;

    tname = sin_wrk_thread_get_tname(&srmtp->t);
    fds.fd = srmtp->netmap_fd;
    fds.events = POLLIN;
    for (;;) {
        if (sin_wrk_thread_check_ctrl(&srmtp->t) == SIGTERM) {
            break;
        }
        nready = poll(&fds, 1, 10);
        if (nready <= 0 || srmtp->first == NULL) {
            continue;
        }
        for (rmcp = srmtp->first; rmcp != NULL; rmcp =  SIN_ITER_NEXT(rmcp)) {
            if (nm_ring_empty(rmcp->rx_ring)) {
                continue;
            }
            rmcp->client_wakeup(rmcp->wakeup_arg);
        }
    }
}

struct sin_ringmon_thread *
sin_ringmon_thread_ctor(const char *tname, int netmap_fd,
  int *e)
{
    struct sin_ringmon_thread *srmtp;

    srmtp = malloc(sizeof(struct sin_ringmon_thread));
    if (srmtp == NULL) {
        _SET_ERR(e, ENOMEM);
        return (NULL);
    }
    memset(srmtp, '\0', sizeof(struct sin_ringmon_thread));
    srmtp->netmap_fd = netmap_fd;

    if (sin_wrk_thread_ctor(&srmtp->t, tname,
      (void *(*)(void *))&sin_ringmon_thread, e) != 0) {
        free(srmtp);
        return (NULL);
    }
    return (srmtp);
}

void
sin_ringmon_thread_dtor(struct sin_ringmon_thread *srmtp)
{

    SIN_TYPE_ASSERT(srmtp, _SIN_TYPE_WRK_THREAD);
    sin_wrk_thread_dtor(&srmtp->t);
    free(srmtp);
}
