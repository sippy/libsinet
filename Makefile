LIB=		sinet
SHLIB_MAJOR=	0
LIBTHREAD?=	pthread

SRCS=		include/libsinet.h sin_close.c libsinet_internal.h sin_socket.c \
		sin_errno.h sin_errno.c sin_bind.c sin_connect.c sin_queue.c \
		sin_init.c sin_stance.h

LDADD=		-l${LIBTHREAD}

NO_PROFILE?=	YES

WARNS?=		4

.include <bsd.lib.mk>
