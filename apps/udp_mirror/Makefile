TOPSRC=${.CURDIR}/../..
SINSRC=${TOPSRC}/src

PROG=		udp_mirror
MAN1=
LIBTHREAD?=	pthread

.if defined(SIN_DEBUG)
CFLAGS+=	-g3 -O0 -DSIN_DEBUG
. if defined(SIN_DEBUG_WAVE)
CFLAGS+=	-DSIN_DEBUG_WAVE=${SIN_DEBUG_WAVE}
. endif
.endif

SRCS=		udp_mirror.c udpm_sorter.h udpm_sorter.c udpm_daemon.c \
		udpm_daemon.h

CFLAGS+=	-I${TOPSRC} -I${SINSRC}
LDADD=		-l${LIBTHREAD} -L${SINSRC} -lsinet

WARNS?=		4

.include <bsd.prog.mk>
