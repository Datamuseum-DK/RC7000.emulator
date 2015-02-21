PROG	=	slavectl

NO_MAN	=	sorry

WARNS   ?=      6

CFLAGS  +=      -I${.CURDIR}/../include -I${.CURDIR} -g -static

LDADD   +=      -L../librc3600 -lrc3600

.include <bsd.prog.mk>

test:	${PROG}
	./${PROG}
