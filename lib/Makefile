PROG=	lagud
NO_MAN=	yes

CFLAGS+=	-DLAGUD_MAIN -I../include

.include <bsd.prog.mk>

magic.c:	magic.sh
	cd ${.CURDIR} && sh magic.sh > magic.c

test:	${PROG}
	./${PROG} < ../rddump/_ 
