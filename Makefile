LIB=		sinet
SHLIB_MAJOR=	0
LIBTHREAD?=	pthread

CFLAGS+=	-DSIN_DEBUG

SRCS=		include/libsinet.h sin_close.c libsinet_internal.h sin_socket.c \
		sin_errno.h sin_errno.c sin_bind.c sin_connect.c sin_queue.c \
		sin_init.c sin_pkt_zone.c sin_pkt_zone.h sin_pkt.c sin_pkt.h \
		sin_type.h sin_wrk_thread.h sin_rx_thread.c sin_stance.h \
		sin_wi_queue.c sin_wi_queue.h sin_signal.c sin_signal.h \
		sin_wrk_thread.c sin_tx_thread.c sin_tx_thread.h

LDADD=		-l${LIBTHREAD}

NO_PROFILE?=	YES

WARNS?=		4

test: lib${LIB}.a test.c
	cc ${CFLAGS} -I. ${LDADD} test.c -o test libsinet.a

.include <bsd.lib.mk>
