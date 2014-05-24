#include <sys/types.h>
#include <sys/socket.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <include/libsinet.h>

int
main(int argc, char **argv)
{
    void *sinp;
    int sin_err;

    sin_err = 0;
    sinp = sin_init("em1", &sin_err);
    if (sinp == NULL) {
        errx(1, "sin_init: %s", strerror(sin_err));
    }

    sleep(60);

    sin_destroy(sinp);

    exit(0);
}
