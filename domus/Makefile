PROG	=	DOMUS

SRCS	+=	main.c

WARNS	?=	6

NO_MAN	=	sorry

CFLAGS	+=	-I${.CURDIR}/../include -I${.CURDIR} -g -static

LDADD	+=	-L../librc3600 -lrc3600

.include <bsd.prog.mk>

test:	${PROG}
	#./${PROG} DOMAC BIN.BB LIST.LL PHMAC PH00S
	sed 's/$$//' src > __.SRC
	./${PROG} DOMAC BIN.BB LIST.LL SRC
	tail -3 __.LL
