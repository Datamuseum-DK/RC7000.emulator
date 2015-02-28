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

slave:	${PROG}
	sed 's/$$//' ../slavectl/slave.src > __.SRC
	./${PROG} DOMAC BIN.SLAVE LIST.LL SRC
	tail -3 __.LL

rxmod:	${PROG}
	cat ../sources/ZMAC ../sources/RXMOS | sed 's/$$//' > __.SRC
	./${PROG} DOMAC BIN.RXMOD LIST.LL SRC
	tail -3 __.LL
	sh prepsrc.sh < __.SRC > /tmp/_
	ls -l __.SRC /tmp/_
