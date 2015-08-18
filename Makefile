LIB=		sinet
SHLIB_MAJOR=	0
LIBTHREAD?=	pthread

.if defined(SIN_DEBUG)
CFLAGS+=	-g3 -O0 -DSIN_DEBUG
. if defined(SIN_DEBUG_WAVE)
CFLAGS+=	-DSIN_DEBUG_WAVE=${SIN_DEBUG_WAVE}
. endif
.endif

SRCS=		include/libsinet.h sin_close.c libsinet_internal.h sin_socket.c \
		sin_errno.h sin_errno.c sin_bind.c sin_connect.c sin_queue.c \
		sin_init.c sin_pkt_zone.c sin_pkt_zone.h sin_pkt.c sin_pkt.h \
		sin_types.h sin_wrk_thread.h sin_rx_thread.c sin_stance.h \
		sin_wi_queue.c sin_wi_queue.h sin_signal.c sin_signal.h \
		sin_wrk_thread.c sin_tx_thread.c sin_tx_thread.h \
		sin_ip4_icmp.c sin_ip4_icmp.h sin_list.h sin_destroy.c \
		sin_debug.h sin_pkt_zone_fast.h sin_mem_fast.h sin_ip4.h \
		sin_pkt_sorter.c sin_pkt_sorter.h

LDADD=		-l${LIBTHREAD}

NO_PROFILE?=	YES

WARNS?=		4

CLEANFILES+=	test

test: lib${LIB}.a test.c Makefile
	cc -O0 -g3 -I. ${LDADD} test.c -o test libsinet.a

includepolice:
	for file in ${SRCS}; do \
	  python misc/includepolice.py $${file} || sleep 5; \
	done

.include <bsd.lib.mk>
