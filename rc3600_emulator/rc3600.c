/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */

/*
 * 015-000023-01_ProgRef.pdf pdf_pg=19:
 *	NOTE: When referencing auto-increment and auto-decrement locations,
 *	the state of bit 0 before the increment or decrement is the condition
 *	upon which the continuation of the indirection chain is based.  For
 *	example: if an auto-increment location contains 177777o, and the
 *	location is referenced as part of an indirection chain, location 0
 *	will be th enext address of the chain.
 *
 * 015-000023-01_ProgRef.pdf pdf_pg=32:
 *	io-dev = 1 contains
 *		push/pop/save/mtsp/mtfp/mfsp/mffp
 * 015-000023-01_ProgRef.pdf pdf_pg=34:
 *	TRAP instruction
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/endian.h>
#include <sys/queue.h>
#include <lagud.h>
#include <termios.h>
#include <sys/select.h>

#include "rc3600_emul.h"
#include "rc3600_io.h"

#include "rc3600_emul_time.c"

static FILE *ftrace;
static uint16_t		tracebuf[200];
static uint16_t		*tracep;

int verbose = 0;
FILE *vfile;

int x11 = 0;

static u_int die, stop, step = 0;
static char vbuf[200], *vbufp, *vbufe;
static uint16_t frontswitch;


static int domus;

uint64_t NINS, NRD, NWR;

/************************************************************************/
/* History state registers						*/
/************************************************************************/

#define NHISTORY 16

static uint16_t h_pc[NHISTORY];
static uint16_t h_instr[NHISTORY];
static uint16_t h_acc0[NHISTORY];
static uint16_t h_acc1[NHISTORY];
static uint16_t h_acc2[NHISTORY];
static uint16_t h_acc3[NHISTORY];
static uint8_t  h_carry[NHISTORY];
static u_int h_p;

/************************************************************************/

void __printflike(1,2)
ioprint(const char *fmt, ...)
{
	va_list ap;

	if (!verbose)
		return;
	va_start(ap, fmt);
	if (vbufp < vbufe)
		*vbufp++ = ' ';
	if (vbufp < vbufe)
		vbufp += vsnprintf(vbufp, vbufe - vbufp, fmt, ap);
	va_end(ap);
}

static void __printflike(1,2)
vprint(const char *fmt, ...)
{
	va_list ap;

	if (!verbose)
		return;
	va_start(ap, fmt);
	if (vbufp < vbufe)
		*vbufp++ = ' ';
	if (vbufp < vbufe)
		vbufp += vsnprintf(vbufp, vbufe - vbufp, fmt, ap);
	va_end(ap);
}

/************************************************************************/

struct event {
	unsigned		count;
	LIST_ENTRY(event)	next;
	event_f			*func;
	void			*arg1;
	int			arg2;
};

static LIST_HEAD(,event)	eventhead = LIST_HEAD_INITIALIZER(eventhead);


void
timeout(unsigned count, event_f func, void *arg1, int arg2)
{
	struct event *ep, *ep2;

	ep = calloc(sizeof *ep, 1);
	if (ep == NULL)
		err(1, "Malloc failed");
	ep->func = func;
	ep->arg1 = arg1;
	ep->arg2 = arg2;
	ep->count = count;
	if (LIST_EMPTY(&eventhead)) {
		LIST_INSERT_HEAD(&eventhead, ep, next);
		return;
	}
	ep2 = LIST_FIRST(&eventhead);
	for (;;) {
		if (ep2->count >= ep->count) {
			ep2->count -= ep->count;
			LIST_INSERT_BEFORE(ep2, ep, next);
			return;
		}
		ep->count -= ep2->count;
		if (LIST_NEXT(ep2, next) == NULL) {
			LIST_INSERT_AFTER(ep2, ep, next);
			return;
		}
		ep2 = LIST_NEXT(ep2, next);
	}
}

/************************************************************************/

static trace_f	*traces[65536];
void
trace_set(uint16_t adr, trace_f *func)
{

	traces[adr] = func;
}

void
trace_reset(uint16_t adr, trace_f *func __unused)
{

	traces[adr] = NULL;
}

/************************************************************************/

static u_int instr_stats[65536];
static u_int core_stats[65536];

#define CORESIZE	(32*1024)

static uint16_t	core[CORESIZE];

FILE *fx;

void
cw(uint16_t a, uint16_t d)
{
	unsigned short t[2];

	if (x11) {
		if (fx == NULL) {
			fx = popen("/critter/nonback/Tmp/Xt/simple-drawing", "w");
			sleep (3);
		}
	}
	if (a >= CORESIZE) {
		fprintf(stderr, "cw Discarded write %04x=%04x\n", a, d);
		if (ftrace != NULL) {
			*tracep++ = 0x0501;
			*tracep++ = a;
		}
		return;
	}
	if (ftrace != NULL) {
		*tracep++ = 0x0602;
		*tracep++ = a;
		*tracep++ = d;
	}
	core[a] = d;
	NWR++;
	if (fx != NULL) {
		t[0] = a;
		t[1] = d;
		fwrite(&t, 2, 2, fx);
		fflush(fx);
	}
}

uint16_t
cr(uint16_t a)
{
	if (a >= CORESIZE) {
		fprintf(stderr, "CR Discarded write %04x\n", a);
		if (ftrace != NULL) {
			*tracep++ = 0x0301;
			*tracep++ = a;
		}
		return (0);
	}
	NRD++;
	if (ftrace != NULL) {
		*tracep++ = 0x0402;
		*tracep++ = a;
		*tracep++ = core[a];
	}
	return (core[a]);
}

static uint16_t	mask, ipen, intr, intr_next;

void
irq_lower(struct iodev *io)
{

	vprint("{Lower %d}", io->unit);
	ipen &= ~(0x8000 >> io->imask);
	io->busy = 0;
	io->done = 0;
	io->ipen = 0;
}

void
dev_irq(void *arg1, int arg2 __unused)
{
	struct iodev *io;

	io = arg1;
	io->ipen = 1;
	if (io->busy) {
		ipen |= (0x8000 >> io->imask);
		vprint("IRQ %d %04x -> %04x M:%04x", io->unit,
		    (0x8000 >> io->imask), ipen, mask);
	}
	io->busy = 0;
	io->done = 1;
}

/* AUTOLOAD ========================================================= */

static uint16_t autorom[64] = {
	0062677,	0060477,	/* 0x00 */
	0060477,	0105120,	/* 0x01 */
	0024026,	0124240,	/* 0x02 */
	0107400,	0010011,	/* 0x03 */
	0124000,	0010031,	/* 0x04 */
	0010014,	0010033,	/* 0x05 */
	0010030,	0010014,	/* 0x06 */
	0010032,	0125404,	/* 0x07 */
	0125404,	0000003,	/* 0x08 */
	0000005,	0060100 - 1,	/* 0x09 */
	0030016,	0030017,	/* 0x0a */
	0050377,	0050377,	/* 0x0b */
	0060077,	0063400 - 1,	/* 0x0c */
	0101102,	0000011,	/* 0x0d */
	0000377,	0101102,	/* 0x0e */
	0004030,	0000377,	/* 0x0f */
	0101065,	0004031,	/* 0x10 */
	0000017,	0101065,	/* 0x11 */
	0004027,	0000020,	/* 0x12 */
	0046026,	0004030,	/* 0x13 */
	0010100,	0046027,	/* 0x14 */
	0000022,	0010100,	/* 0x15 */
	0000077,	0000023,	/* 0x16 */
	0126420,	0000077,	/* 0x17 */
	0063577,	0126420,	/* 0x18 */
	0000030,	0063600 - 1,	/* 0x19 */
	0060477,	0000031,	/* 0x1a */
	0107363,	0060500 - 1,	/* 0x1b */
	0000030,	0107363,	/* 0x1c */
	0125300,	0000031,	/* 0x1d */
	0001400,	0125300,	/* 0x1e */
	0000000,	0001400,	/* 0x1f */
};

/* From RCSL 52-AA894, Appendix F */
static uint16_t autorom_fd[32] = {
		     // ; PROGRAM LOAD, FLEXIBLE DISC, HKM 75.11.01
		     // ;  THIS PROGRAM LOAD RESIDES IN 32x16 ROM.
		     // ;  IT IS DESIGNED FOR FLEXIBLE DISC AS PRIMARY LOAD MEDIUM
		     // ;     AND USES MOVING HEAD DISC OR MAGTAPE AS SECONDARY
		     // ;     LOAD MEDIUM.
		     // ;
		     // ;  FLEXIBLE DISC: SWITCH(0) = 0, SWITCH(1:15) - NOT USED,
		     // ;                 THE DISC IS RECALIBRATED BY THE PROGRAM
		     // ;  MAGTAPE:
		     // ;  MOVING HEAD DISC: SWITCH(0) = 1, SWITCH(1:9) = 0
		     // ;                 SWITCH(10:15) = DEVICE NUMBER,
		     // ;                 BOTH DISC AND MAGTAPE MUST BE RECALIBRATED
		     // ;                 BEFORE ACTIVATING THE PROGRAM LOAD
		     // ;
		     // ;  IN CASE OF MAGTAPE OF FLEXIBLE DISC THE LOAD WAITS UNTIL
		     // ;     THE SELECTED DEVICE IS READY FOR COMMANDS.
		     // .LOC	0
		     // FLEX=	61		; FLEXIBLE DISC
/* 00000 */ 0070477, //		READS	2	; READ SWITCHES(S);
/* 00001 */ 0150122, //		COMZL	2,2,SZC	; IF S(0) == 0 THEN
/* 00002 */ 0000026, //		JMP	FD	;   CARRY:= TRUE AND GOTO FLOP
/* 00003 */ 0151240, //		MOVOR	2,2	; NOT FLOPPY: DEVICE:= OCT(77);
/* 00004 */ 0010010, // LOOP:	ISZ	OP1	; FOR DEVICE INDEX:=S(1:15)-1
/* 00005 */ 0010013, //		ISZ	OP2	;   STEP 1 UNTIL 0 DO
/* 00006 */ 0151404, //		INC	2,2,SZR	;   DEVICE:= DEVICE +1
/* 00007 */ 0000004, //		JMP	LOOP	; FOR FURTHER COMMENTS SEE OP1
/* 00010 */ 0071077, // OP1:	071077		; DOAS 2 <DEV> -1 :INCREMENTS:
/* 00011 */ 0024015, //		LDA	1,.377	; LOAD "JMP .+0" INTO LAST WORD
/* 00012 */ 0044377, //		STA	1,377	;   OF PAGE ZERO
/* 00013 */ 0063377, // OP2:	0633377		; SKPBN <DEV> -1 :INCREMENTS:
/* 00014 */ 0000010, //		JMP	OP1	; READ FIRST BLOCK, WAIT UNTIL
		     //				;   COMMAND IS ACCEPTED
/* 00015 */ 0000377, // .377:	JMP	377	; GOTO WAIT BLOCK TRANSFERRED
/* 00016 */ 0126420, // READN:	SUBZ	1,1	; GETWORDS: WORD:=0; CARRY=TRUE
/* 00017 */ 0061461, //		DIB	0,FLEX	; READ(CHAR)
/* 00020 */ 0107363, //		ADDCS	0,1,SNC	; WORD:= WORD SHIFT 8 + CHAR;
/* 00021 */ 0000017, //		JMP	READN+1	; IF CARRY = FALSE THEN READ CH
/* 00022 */ 0046025, //		STA@	1,ADR	; INCR(ADR); CORE(ADR) += WORD
/* 00023 */ 0010100, //		ISZ	100	; IF INCR(CORE(100)) <> 0 THEN
/* 00024 */ 0000016, //		JMP	READN	; READ NEXT WORD ELSE
/* 00025 */ 0000077, //	ADR:	JMP	77	; GOTO ADR
		     // ; FLEXIBLE DISK: AT ENTRY, CARRY == TRUE!!
/* 00026 */ 0030037, // FD:	LDA	2,COMM	; FLOPPY: COMMAND:=RECALIBRATE
/* 00027 */ 0071161, // EXE:	DOAS	2,FLEX	; EXECUTE: EXECUTE(COMMAND)
/* 00030 */ 0063461, //		SKPBN	FLEX	; ! COMMAND(0:7) = DONT CARE !
/* 00031 */ 0000027, //		JMP	EXE	; WAIT UNTIL COMMAND ACCEPTED
/* 00032 */ 0063661, //		SKPDN	FLEX	; WAIT UNTIL COMMAND EXECUTED
/* 00033 */ 0000032, //		JMP	.-1	;
/* 00034 */ 0151102, //		MOVL	2,2,SZC	; IF NEXT COMMAND = READ BLOCK
/* 00035 */ 0000027, //		JMP	EXE	;   THEN GOTO EXECUTE ELSE
/* 00036 */ 0000016, //		JMP	READN	;   GOTO GETWORDS
/* 00037 */ 0101000, // COMM:	1B0+1B6		; COMMAND BITS
};

/* RC3803 =========================================================== */

static void
dev_rc3803(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev __unused)
{
	uint16_t w;

	switch(ioi) {
	case 062600:	// LDB
		w = cr(acc[1] >> 1);
		if (acc[1] & 1)
			acc[0] = w & 0xff;
		else
			acc[0] = w >> 8;
		return;
	default:
		printf("\r\n\nUnhandled RC3803 instruction: %06o\r\n", ioi);
		exit(0);
	}
}

/* CPU ============================================================== */

static void
dev_cpu(uint16_t ioi, uint16_t *reg, struct iodev *iodev __unused)
{
	int i;
	unsigned u;

	switch (ioi) {
	case SKPBN:
		vprint("SKP INT_ON");
		if (intr)
			npc++;
		break;
	case SKPBZ:
		vprint("SKP !INT_ON");
		if (!intr)
			npc++;
		break;
	case SKPDN:
		vprint("SKP POWERFAIL");
		break;
	case SKPDZ:
		vprint("SKP !POWERFAIL");
		npc++;
		break;
	case NIOS:
		vprint("INTEN");
		intr_next = 1;
		break;
	case NIOC:
		vprint("INTDS");
		intr = 0;
		break;
	case DIA:
		vprint("READS %o", frontswitch);
		*reg = frontswitch;
		break;
	case DIC:
	case DICC:
#if 0
		/*
		 * Patch up CATW in the diablo image:
		 * Unit 8 has offset 70
		 * Unit 9 has driver FD1 unit 0 offset 0
		 */
		cw(0xc7d, 70);		/* Unit 8 offset */
		cw(0xc86, 0x0c8d);	/* Unit 9 driver */
		cw(0xc8e, 0x3100);	/* ditto */
		cw(0xc90, 0);		/* Unit 9 unit */
		cw(0xc91, 0);		/* Unit 9 offset */
#endif
		vprint("IORST");
		for(i = 0; i < 63; i++) {
			iodevs[i].ipen = 0;
			iodevs[i].busy = 0;
			iodevs[i].done = 0;
			if (iodevs[i].func != NULL)
				irq_lower(&iodevs[i]);
		}
		ipen = 0;
		mask = 0;
		break;
	case DOB:
		vprint("MSKO");
		mask = *reg;
		break;
	case DIB:
		vprint("INTA");
		*reg = 0;
		u = 16;
		for(i = 0; i < 63; i++) {
			if (!iodevs[i].ipen)
				continue;
			if ((0x8000 >> iodevs[i].imask) & ~mask &&
			    iodevs[i].imask < u) {
				u = iodevs[i].imask;
				*reg = i;
			}
		}
		if (u == 16) {
			*reg = 0;
			vprint("INTA: no irq pending");
		} else {
			vprint("INTA: %d", *reg);
		}
		break;
	case DOC:
		fprintf(stderr, "\r\nHALT at 0x%04x 0%06o\n\r", npc, npc);
		stop = 1;
		vprint("HALT");
		break;
	default:
		fprintf(stderr,
		    "\r\nUNKNOWN CPU IO 0x%04x at 0x%04x 0%06o\n\r",
		    ioi, npc, npc);
		break;
	}
}

static int
config_cpu(char **ap)
{
	int i;

	if (strcmp(ap[0], "cpu"))
		return (0);
	if (!strcmp(ap[1], "rc3803")) {
		iodevs[1].func = &dev_rc3803;
		return (1);
	}
	if (!strcmp(ap[1], "config")) {
		iodevs[63].func = &dev_cpu;
		return (1);
	}
	if (!strcmp(ap[1], "switch")) {
		frontswitch = strtoul(ap[2], NULL, 0);
		fprintf(stderr, "switch = %04x\n", frontswitch);
		return (1);
	}
	if (!strcmp(ap[1], "autorom_fd")) {
		for (i = 0; i < 32; i++)
			cw(i, autorom_fd[i]);
		npc = 0;
		return (1);
	}
	if (!strcmp(ap[1], "autoload")) {
		for (i = 0; i < 32; i++)
			cw(i, autorom[i * 2 + 1]);
		npc = 0;
		return (1);
	}
	if (!strcmp(ap[1], "start")) {
		if (ap[2] != NULL)
			pc = strtoul(ap[2], NULL, 0);
		stop = 0;
		return (1);
	}
	return (1);
}

#if 0
/* OBJ LOADER ======================================================= */

static void
obj_loader(const char *fn)
{
	FILE *f;
	uint16_t l, a, s, u;
	int i;

	f = fopen(fn, "r");
	if (f == NULL)
		err(1, "Cannot open %s", fn);

	for (;;) {
		for (;;) {
			i = getc(f);
			if (i == EOF)
				errx(1, "EOF on obj_load file %s", fn);
			if (i)
				break;
		}
		l = i;
		l |= getc(f) << 8;
		if (!(l & 0x8000))
			break;
		s = l;
		a = getc(f); a |= getc(f) << 8; s += a;
		pc = a;
		u = getc(f); u |= getc(f) << 8; s += u;
		while (l) {
			u = getc(f); u |= getc(f) << 8; s += u;
			cw(a++, u);
			l++;
		}
		if (s != 0)
			errx(1, "Checksum error: %04x", s);
	}
	fclose(f);
}
#endif

/* MAIN ============================================================= */

static int
xmain(void)
{
	char dbuf[140];
	uint16_t u, opc;
	int i;
	u_int *next_event;
	struct timeval t1, t2;
	struct event *ep;

	vbufe = vbuf + sizeof vbuf - 1;

	ipen = intr = mask = carry = 0;
	acc[0] = acc[1] = acc[2] = acc[3] = pc = 0;

	for (i = 0; i < 64; i++) {
		if (iodevs[i].func == NULL)
			iodevs[i].func = dev_nodev;
		iodevs[i].unit = i;
	}
	gettimeofday(&t1, NULL);
	for(NINS = 0; !die; NINS++) {
		vbufp = vbuf;
		*vbuf = '\0';

		/* Handle timed events */
		while (1) {
			if (dur == 0)
				break;
			ep = LIST_FIRST(&eventhead);
			if (ep == NULL)
				break;
			if (ep->count <= dur) {
				dur -= ep->count;
				LIST_REMOVE(ep, next);
				ep->func(ep->arg1, ep->arg2);
				if (verbose)
					fprintf(vfile, "%10jd Event: %p(%p, %d) %s\n",
						NINS,
						ep->func, ep->arg1, ep->arg2,
						vbuf);
				free(ep);
				continue;
			}
			ep->count -= dur;
			dur = 0;
			break;
		}
		*vbuf = '\0';

		if (stop) {
			dur += 10000 * 1000;
			usleep(10000);
			continue;
		}

		/* Handle interrupts */
		if (intr && (ipen & ~mask)) {
			ioprint("%10jd IO:IRQ: intr=%d, ipen=%04x, mask=%04x",
			     NINS, intr, ipen, mask);
			intr = 0;
			cw(0, pc);
			pc = cr(1);
		}
		if (intr_next) {
			intr = 1;
			intr_next = 0;
		}

		if (ftrace != NULL) {
			*tracep++ = 0x0006;
			*tracep++ = pc;
			*tracep++ = acc[0];
			*tracep++ = acc[1];
			*tracep++ = acc[2];
			*tracep++ = acc[3];
			*tracep++ = carry;
		}

		ci = cr(pc);

		/* Record history */
		h_pc[h_p] = pc;
		h_instr[h_p] = ci;
		h_acc0[h_p] = acc[0];
		h_acc1[h_p] = acc[1];
		h_acc2[h_p] = acc[2];
		h_acc3[h_p] = acc[3];
		h_carry[h_p] = carry;
		h_p++;
		h_p %= NHISTORY;

		ep = LIST_FIRST(&eventhead);
		if (ep != NULL)
			next_event = &ep->count;
		else
			next_event = NULL;
		if (traces[pc] != NULL)
			traces[pc](pc, 0, next_event);

		if (traces[ci] != NULL)
			traces[ci](ci, 1, next_event);

		step = 0;

		if (ftrace != NULL) {
			*tracep++ = 0x0201;
			*tracep++ = ci;
		}
		core_stats[pc]++;
		instr_stats[ci]++;
		opc = pc;
		rc3600_exec();
		if (ftrace != NULL) {
			*tracep++ = 0x0106;
			*tracep++ = acc[0];
			*tracep++ = acc[1];
			*tracep++ = acc[2];
			*tracep++ = acc[3];
			*tracep++ = carry;
			*tracep++ = npc;
			fwrite(tracebuf, sizeof *tracebuf, tracep - tracebuf, ftrace);
			tracep = tracebuf;
		}

		if (die || verbose) {
			LagudDisass(dbuf, ci, NULL, domus);
			fprintf(vfile, "%10jd %04x:%04x", NINS, opc, ci);
			fprintf(vfile, " %04x %04x %04x %04x %s ",
			    acc[0], acc[1], acc[2] ,acc[3], carry ? "C" : ".");
			fprintf(vfile, " |%-16s| ", dbuf);
			*vbufe = '\0';
			fprintf(vfile, "%04x %04x %04x %04x %s [%s]\n",
			    acc[0], acc[1], acc[2] ,acc[3],
			    carry ? "C" : ".", vbuf);
			if (step)
				fflush(vfile);
		}
		pc = npc & 0x7fff;
		if (stop)
			printf("Stopped, next PC: 0x%04x\r\n", pc);
		if (die)
			break;
	}
	gettimeofday(&t2, NULL);

	{
	double a;

	a = t2.tv_sec - t1.tv_sec;
	a += 1e-6 * (t2.tv_usec - t1.tv_usec);

	fprintf(stderr, "\r\n\n%.6f seconds %jd instructions %.0f instructions/sec\n",
		a, NINS, NINS/a);
	}

	{
	FILE *fo;

	fo = fopen("/tmp/_.core", "w");
	if (fo == NULL)
		err(1, "cannot open /tmp/_.core");
	for (i = 0; i < CORESIZE; i++) {
		u = cr(i);
		fputc(u >> 8, fo);
		fputc(u & 0xff, fo);
	}
	fclose(fo);
	}

	{
	FILE *fo;

	fo = fopen("/tmp/_.instr_stats", "w");
	if (fo == NULL)
		err(1, "cannot open /tmp/_.instr_stats");
	for (i = 0; i < 65536; i++)
		if (instr_stats[i]) {
			LagudDisass(dbuf, i, NULL, domus);
			fprintf(fo, "%10u %5d %04x %s\n",
			    instr_stats[i], i, i, dbuf);
		}
	fclose(fo);
	}
	{
	FILE *fo;

	fo = fopen("/tmp/_.core_stats", "w");
	if (fo == NULL)
		err(1, "cannot open /tmp/_.instr_stats");
	for (i = 0; i < 32768; i++)
		if (core_stats[i]) {
			LagudDisass(dbuf, core[i], NULL, domus);
			fprintf(fo, "%10u %5d %04x %04x %s\n",
			    core_stats[i], i, i, core[i], dbuf);
		}
	fclose(fo);
	}

	{
	for (i = 0; i < NHISTORY; i++) {
		fprintf(stderr, "%04x %04x [%04x %04x %04x %04x %s]",
		    h_pc[h_p], h_instr[h_p], h_acc0[h_p], h_acc1[h_p],
		    h_acc2[h_p], h_acc3[h_p], h_carry[h_p] ? "C " : "NC");
		LagudDisass(dbuf, h_instr[h_p], NULL, domus);
		fprintf(stderr, " %s\n\r", dbuf);
		h_p++;
		h_p %= NHISTORY;
	}
	}

	return (0);
}

struct devconf {
	ioconf_f		*config;
	LIST_ENTRY(devconf)	next;
};

static LIST_HEAD(, devconf) devconfhead = LIST_HEAD_INITIALIZER(&devconfhead);

static void
AddDev(ioconf_f	*func)
{
	struct devconf *dc;

	dc = calloc(sizeof *dc, 1);
	if (dc == NULL)
		err(1, "malloc failed");
	dc->config = func;
	LIST_INSERT_HEAD(&devconfhead, dc, next);
}

static int
Cmd(const char *pp)
{
	char **ap, *av[100], *p;
	struct devconf *dc;

	p = strdup(pp);
	fprintf(stderr, "--> %s\n", p);
	ap = av;
	while ((*ap = strsep(&p, " \t")) != NULL) {
		if (**ap != '\0')
			if (++ap >= &av[100])
				break;
	}
	LIST_FOREACH(dc, &devconfhead, next) {
		if (dc->config(av)) {
			free(p);
			return (0);
		}
	}
	fprintf(stderr, "Not handled: ");
	for(ap = av; *ap; ap++)
		fprintf(stderr, " %s", *ap);
	fprintf(stderr, "\n");
	free(p);
	return (1);
}


static struct termios ttyi_save;
static int ttyi_istty;

static void
CLI_init_tty(void)
{
	struct termios t;

	ttyi_istty = isatty(0);
	if (!ttyi_istty)
		return;
	tcgetattr(0, &ttyi_save);
	tcgetattr(0, &t);
	cfmakeraw(&t);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &t);
}

static void
CLI_finish_tty(void)
{

	if (!ttyi_istty)
		return;
	tcsetattr(0, TCSANOW, &ttyi_save);
}

static char *ta;
static int lta, nta, rta, has_cmd;

static void
CLI_input(int chr)
{

	if (ta == 0) {
		lta = BUFSIZ;
		ta = malloc(BUFSIZ);
	}
	if (chr >= 0) {
		if (has_cmd) {
			if (chr == '\b') {
				if (ta[nta - 1] != '\001') {
					nta--;
					if (ttyi_istty)
						fprintf(stderr, "\b \b");
				}
				return;
			}
			if (ttyi_istty && chr >= ' ' && chr <= '~')
				fprintf(stderr, "%c", chr);
			if (ttyi_istty && (chr == '\n' || chr == '\r'))
				fprintf(stderr, "\r\n");
		}
		if (!ttyi_istty && chr == '\n')
			chr = '\r';

		ta[nta++] = chr;
		if (chr == '\001') {
			has_cmd = 1;
			if (ttyi_istty)
				fprintf(stderr, "[CMD] ");
		}
	}
	if (nta == lta) {
		lta += lta;
		ta = realloc(ta, lta);
	}
	if (ta[rta] != '\001' && TTYI_Input(ta[rta])) {
		rta++;
		if (rta == nta)
			rta = nta = 0;
	}
	if (ta[rta] == '\001' && ta[nta - 1] == '\r') {
		ta[nta - 1] = '\0';
		CLI_finish_tty();
		Cmd(ta + 1);
		CLI_init_tty();
		rta = nta = 0;
		has_cmd = 0;
	}
}

static void
CLI_timeout(void *arg __unused, int flag __unused)
{
	fd_set rfd, wfd, efd;
	struct timeval tv;
	int maxfd, i, j;
	char c;

	FD_ZERO(&rfd);
	FD_ZERO(&wfd);
	FD_ZERO(&efd);
	FD_SET(0, &rfd);
	FD_SET(0, &efd);
	FD_SET(1, &efd);
	maxfd = 1;

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	i = select(maxfd + 1, &rfd, &wfd, &efd, &tv);
	if (verbose)
		fprintf(vfile, "Sel(%d.%06ld) = %d\n",
		    (int)tv.tv_sec, tv.tv_usec, i);
	if (i > 0 && (FD_ISSET(0, &rfd) || FD_ISSET(0, &efd))) {
		j = read(0, &c, 1);
		if (j == 1) {
			if (c == '\003')
				die = 1;
			else
				CLI_input(c);
		} else if (nta == 0) {
			fprintf(stderr, "Exception stdin\n");
			die = 1;
		}
	}
	if (nta > 0)
		CLI_input(-1);
	timeout(10000000, CLI_timeout, NULL, 0);
}

int
main(int argc, char **argv)
{
	int c;

	vfile = stderr;
	timing = nova1200_timing;
	timing = nova2_timing;
	AddDev(config_cpu);
	AddDev(config_dkp);
	AddDev(config_rtc);
	AddDev(config_tty);
	AddDev(config_tty2);
	AddDev(config_ptp);
	AddDev(config_ptr);
	AddDev(config_rc3751);
	AddDev(config_domus);
	AddDev(config_lpt);
	AddDev(config_amx);
	AddDev(config_floppy);
	AddDev(config_stat);

	Cmd("cpu config");
	Cmd("tty config");
	Cmd("rtc config");

	while ((c = getopt(argc, argv, "6ac:dt:vx")) != -1) {
		switch (c) {
		case 'a':
			Cmd("cpu autoload");
			break;
		case '6':
			Cmd("dkp config");
			Cmd("cpu switch 0100073");
			break;
		case 'c':
			Cmd(optarg);
			break;
		case 'd':
			domus = 1;
			Cmd("dkp config");
			Cmd("ptp config");
			Cmd("ptr config");
			Cmd("domus sleep");
			Cmd("domus msg /tmp/_.msg");
			Cmd("cpu switch 0100073");
			break;
		case 't':
			ftrace = fopen(optarg, "w");
			if (ftrace == NULL)
				err(1, "cannot open %s", optarg);
			tracep = tracebuf;
			break;
		case 'v':
			verbose++;
			vfile = fopen("/tmp/_.v", "w");
			if (vfile == NULL)
				err(1, "Cannot open /tmp/_.v");
			break;
		case 'x':
			x11++;
			break;
		default:
			errx(1, "Usage!");
		}
	}

	CLI_init_tty();
	CLI_timeout(NULL, 0);
	xmain();
	CLI_finish_tty();
	return (0);
}
