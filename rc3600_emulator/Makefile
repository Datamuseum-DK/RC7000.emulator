# $Id: Makefile,v 1.36 2010/08/15 14:44:26 phk Exp $

PROG	=	rc3600

SRCS	+=	rc3600.c
SRCS	+=	lagud.c
SRCS	+=	io_stat.c
SRCS	+=	io_dkp.c
SRCS	+=	io_floppy.c
SRCS	+=	io_lpt.c
SRCS	+=	io_ptp.c
SRCS	+=	io_ptr.c
SRCS	+=	io_rc3751.c
SRCS	+=	io_rtc.c
SRCS	+=	io_tty.c
SRCS	+=	io_tty2.c
SRCS	+=	io_amx.c

SRCS	+=	os_domus.c

VPATH	=	${.CURDIR}/../lib

WARNS	?=	5

CFLAGS	+=	-I${.CURDIR}/../include -I${.CURDIR} -g -static

LDADD	+=	-L../librc3600 -lrc3600

NO_MAN	=	yes.

DKP	=	/home/phk/DDHF/DDHF/Rc3600/DKP

.include <bsd.prog.mk>

rc6000:	${PROG}
	./${PROG} -6 -c 'dkp 0 load ../DKP/000/__' -c 'dkp 1 load ../DKP/001/__'
	
comal:	${PROG}
	./${PROG} -d \
		-c 'dkp 0 load ${DKP}/012/__' \
		-c 'amx config 42' \
		-c 'tty speed 1000000' \
		-c 'stat config' \
		-c 'lpt config' \
		-c 'cpu autoload'

domus:	${PROG}
	./${PROG} -d \
		-c 'dkp 0 load ${DKP}/004/__' \
		-c 'tty speed 1000000' \
		-c 'stat config' \
		-c 'lpt config' \
		-c 'cpu rc3803' \
		-c 'cpu autoload'

testv:	${PROG}
	./${PROG} -d -v -c 'dkp 0 load ../DKP/011/__' -c 'dkp 1 load ../DKP/003/__' -c 'dkp 2 load ../DKP/008/__'  > /tmp/_

flint:	${SRCS}
	cd ${.CURDIR} && flexelint -I${.CURDIR}/../include flint.lnt $>

balk03:	${PROG}
	./${PROG} -c 'ptr config' -c 'cpu switch 10' \
		-c 'ptr file ../PTR/balk/03/tape.flx'


ptrtest:	${PROG}
	rm -f /tmp/_p
	#echo -n x > /tmp/_p
	#cat ../PTR/balk/13/tape.flx >> /tmp/_p
	cat ../PTR/balk/62/tape.flx >> /tmp/_p
	#cat ../PTR/095-000038-00 >> /tmp/_p
	./${PROG} -t /tmp/_.tb \
		-c 'ptr config' \
		-c 'cpu switch 10' \
		-c 'tty speed 1200' \
		-c 'ptr file /tmp/_p' \
		-c 'cpu autoload' > /tmp/_

fdtest:	${PROG}
	./${PROG} -c 'floppy config' -c 'cpu switch 0x0031' \
		-c 'floppy load ../FD/sg0018.flp' \
		-c 'cpu autorom_fd' 

fd2test:	${PROG}
	./${PROG} -c 'rc3751 config' -c 'cpu switch 0x8031' \
		-c 'rc3751 0 load ../FD/FDC20050907/FD004/_1' \
		-c 'cpu autoload' -c 'domus msg' -c 'domus sleep'

# -v | grep 'IO(49)' > /tmp/_
dkp009:	${PROG}
	./${PROG} -d -c 'dkp 0 load ../DKP/009/__' \
		-c 'cpu autoload' -c 'domus msg' -c 'domus sleep'

t2:	${PROG}
	./${PROG} -d \
		-c "tty2 1 config" \
		-c "cpu switch 050" \
		-c "cpu rc3803" \
		-c "cpu autoload"
