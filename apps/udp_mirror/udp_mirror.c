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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <include/libsinet.h>

#include "sin_sorter.h"
#include "udpm_sorter.h"

static void
usage(int eval)
{

    printf("Usage: udp_mirror [-t timeout] if0[ if1[.. ifN]]\n");
    exit(eval);
}

int
main(int argc, char **argv)
{
    void **sinp;
    int sin_err, i;
    int tout, ch;
    struct udpm_params args;

    tout = 600;
    while ((ch = getopt(argc, argv, "t:")) != -1) {
        switch (ch) {
        case 't':
            tout = atoi(optarg);
            if (tout < 0) {
                errx(1, "timeout needs to be non-negative");
             }
             break;
        case '?':
             usage(0);
        default:
             usage(1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc < 1) {
        errx(1, "at least one interface name is required");
    }

    sinp = malloc(sizeof(void *) * argc);
    for (i = 0; i < argc; i++) {
        sin_err = 0;
        sinp[i] = sin_init(argv[i], &sin_err);
        if (sinp[i] == NULL) {
            errx(1, "sin_init(%s): %s", argv[i], strerror(sin_err));
        }
        if (sin_sorter_reg(sinp[i], sin_ip4_udp_taste, sin_ip4_udp_proc, &args,
          &sin_err) != 0) {
            errx(1, "sin_sorter_reg(%s): %s", argv[i], strerror(sin_err));
        }
    }

    sleep(tout);

    for (i = 0; i < argc; i++) {
        sin_destroy(sinp[i]);
    }

    exit(0);
}
