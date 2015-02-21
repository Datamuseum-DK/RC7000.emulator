PROG	=	DOMUS

SRCS	+=	main.c

WARNS	?=	6

NO_MAN	=	sorry

CFLAGS	+=	-I${.CURDIR}/../include -I${.CURDIR} -g -static

LDADD	+=	-L../librc3600 -lrc3600

.include <bsd.prog.mk>

test:	${PROG}
	sed 's/$$//' ../sources/RCSL_52AA_894_appendix_d.src > __.SRC
	./${PROG} DOMAC BIN.BB LIST.LL SRC
	tail -3 __.LL
