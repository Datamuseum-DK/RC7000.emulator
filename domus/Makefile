PROG	=	DOMUS

SRCS	+=	main.c

WARNS	?=	6

NO_MAN	=	sorry

CFLAGS	+=	-I${.CURDIR}/../include -I${.CURDIR} -g -static

LDADD	+=	-L../librc3600 -lrc3600

.include <bsd.prog.mk>

test:	${PROG}
	./${PROG} DOMAC BIN.BB LIST.LL PHMAC PH00S
	# ./${PROG} DOMAC BIN.BB LIST.LL SRC
	# ./${PROG} DOMXR DOMST
