/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $Id: rc3600_emul.c,v 1.5 2007/02/25 18:49:49 phk Exp $
 */

#include <assert.h>
#include <sys/endian.h>
#include "rc3600_emul.h"
#include "rc3600_emul_priv.h"

uint16_t	acc[4];			/* The accumulators */
uint8_t		carry;			/* Carry bit */
uint16_t	pc;			/* Program counter */
uint16_t	npc;			/* Next program counter */
uint16_t	ci;			/* Current instruction */

unsigned	*timing;		/* Timing spec, if any. */
unsigned	dur;			/* Duration of current instruction */

struct iodev iodevs[64];

static unsigned notiming[TIME_LASTONE];

/* NODEV============================================================= */

void
dev_nodev(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{

	(void)iodev;
	switch(ioi) {
	case DIA: case DIAS: case DIAC: case DIAP:
	case DIB: case DIBS: case DIBC: case DIBP:
	case DIC: case DICS: case DICC: case DICP:
		*reg = 0;
		return;
	case SKPBZ:
	case SKPDZ:
		npc++;
		return;
	default:
		return;
	}
}

/* ALU Instructions --------------------------------------------------*/

static void
Insn_ALU(void)
{
	unsigned tc, u;
	uint16_t *rps, *rpd, t;
	uint32_t tt;

	switch((ci >> 4) & 3) {
	case 0:	tc = carry;	break;
	case 1:	tc = 0;		break;
	case 2:	tc = 1;		break;
	case 3:	tc = 1 - carry;	break;
	default:
		assert(0 == __LINE__);
		tc = 0;	/* XXX GCC sux */
	}
	rps = &acc[(ci >> 13) & 0x3];
	rpd = &acc[(ci >> 11) & 0x3];
	switch (ci & 0x0700) {
	case 0x0000:	/* COM */
		dur += timing[TIME_ALU_1];
		t = ~(*rps);
		break;
	case 0x0100:	/* NEG */
		dur += timing[TIME_ALU_1];
		t = -(*rps);
		if (*rps == 0)
			tc ^= 1;
		break;
	case 0x0200:	/* MOV */
		dur += timing[TIME_ALU_1];
		t = (*rps);
		break;
	case 0x0300:	/* INC */
		dur += timing[TIME_ALU_1];
		t = (*rps) + 1;
		if (*rps == 0xffff)
			tc ^= 1;
		break;
	case 0x0400:	/* ADC */
		dur += timing[TIME_ALU_2];
		t = ~(*rps) + (*rpd);
		if (*rpd > *rps)
			tc ^= 1;
		break;
	case 0x0500:	/* SUB */
		dur += timing[TIME_ALU_2];
		t = (*rpd) - (*rps);
		if (*rpd >= *rps)
			tc ^= 1;
		break;
	case 0x0600:	/* ADD */
		dur += timing[TIME_ALU_2];
		tt = *rps;
		tt += *rpd;
		if (tt & (1<<16))
			tc ^= 1;
		t = tt;
		break;
	case 0x0700:	/* AND */
		dur += timing[TIME_ALU_2];
		t = (*rps) & (*rpd);
		break;
	default:
		assert(0 == __LINE__);
		t = 0;	/* XXX GCC sux */
	}
	switch((ci >> 6) & 3) {
	case 0:
		break;
	case 1:
		tt = t;
		tt <<= 1;
		tt |= tc & 1;
		tc = (tt >> 16) & 1;
		t = tt;
		break;
	case 2:
		tt = t;
		tt |= tc << 16;
		tc = tt & 1;
		t = tt >> 1;
		break;
	case 3:
		t = bswap16(t);
		break;
	default:
		assert(0 == __LINE__);
	}
	u = 0;
	switch(ci & 7) {
	case 0: break;				/* "   " */
	case 1:	u++; break;			/* SKP */
	case 2: if (!tc) u++; break;		/* SZC */
	case 3: if (tc) u++; break;		/* SNC */
	case 4: if (t == 0) u++; break;		/* SZR */
	case 5: if (t != 0) u++; break;		/* SNR */
	case 6: if (!tc || t == 0) u++; break;	/* SEZ */
	case 7: if (tc && t != 0) u++; break;	/* SBN */
	}
	if (u) {
		dur += timing[TIME_ALU_SKIP];
		npc++;
	}
	if (!(ci & 0x8)) {
		*rpd = t;
		carry = tc;
	}
}

/* I/O Instructions --------------------------------------------------*/

static void
Insn_IO(void)
{
	uint16_t *rpd, ioi;
	struct iodev *iop;
	int unit;

	unit = ci & 0x3f;
	ioi = ci & 0xffc0;
	rpd = &acc[(ci >> 11) & 0x3];
	iop = &iodevs[unit];
	switch (ioi) {
	case SKPBN:
		dur += timing[TIME_IO_SKP];
		if (iop->busy) {
			dur += timing[TIME_IO_SKP_SKIP];
			npc++;
		}
		return;
	case SKPBZ:
		dur += timing[TIME_IO_SKP];
		if (!iop->busy) {
			dur += timing[TIME_IO_SKP_SKIP];
			npc++;
		}
		return;
	case SKPDN:
		dur += timing[TIME_IO_SKP];
		if (iop->done) {
			dur += timing[TIME_IO_SKP_SKIP];
			npc++;
		}
		return;
	case SKPDZ:
		dur += timing[TIME_IO_SKP];
		if (!iop->done) {
			dur += timing[TIME_IO_SKP_SKIP];
			npc++;
		}
		return;
	default:
		break;
	}
	if (ioi & 0x00c0)
		dur += timing[TIME_IO_SCP];
	switch (ioi & 0xe700) {
	case NIO:
		dur += timing[TIME_IO_NIO];
		break;
	case DIA:
	case DIB:
	case DIC:
		dur += timing[TIME_IO_INPUT];
		ioi &= 0xe7ff;
		break;
	case DOA:
	case DOB:
	case DOC:
		dur += timing[TIME_IO_OUTPUT];
		ioi &= 0xe7ff;
		break;
	default:
		assert(0 == __LINE__);
	}
	iop->func(ioi, rpd, iop);
}

static uint16_t
EA(void)
{
	int8_t displ;
	uint16_t t, u;
	int i;

	displ = ci;
	switch((ci >> 8) & 3) {
	case 0:
		t = (uint8_t)displ;
		break;
	case 1:
		t = pc + displ;
		break;
	case 2:
		dur += timing[TIME_BASE_REG];
		t = acc[2] + displ;
		break;
	case 3:
		dur += timing[TIME_BASE_REG];
		t = acc[3] + displ;
		break;
	default:
		assert(0 == __LINE__);
		t = 0; /* XXX: GCC sux */
	}
	t &= 0x7fff;
	/* @ bit */
	i = ci & 0x400;
	while (i) {
		dur += timing[TIME_INDIR_ADR];
		u = cr(t);
		i = u & 0x8000;
		if (t >= 020 && t <= 027) {
			dur += timing[TIME_AUTO_IDX];
			cw(t, ++u);
		} else if (t >= 030 && t <= 037) {
			dur += timing[TIME_AUTO_IDX];
			cw(t, --u);
		}
		t = u & 0x7fff;
	}
	return (t);
}

void
rc3600_exec(void)
{
	uint16_t t, u, *rpd;
	ci = cr(pc);

	assert((carry & ~1) == 0);
	if (timing == (void*)0)
		timing = notiming;
	npc = pc + 1;
	switch((ci >> 13) & 7) {
	case 0:	/* DSZ, ISZ, JMP, JSR */
		t = EA();
		switch((ci >> 11) & 3) {
		case 0:	/* JMP */
			dur += timing[TIME_JMP];
			npc = t;
			break;
		case 1: /* JSR */
			dur += timing[TIME_JSR];
			acc[3] = npc;
			npc = t;
			break;
		case 2: /* ISZ */
			dur += timing[TIME_ISZ];
			u = cr(t);
			cw(t, ++u);
			if (u == 0) {
				dur += timing[TIME_ISZ_SKP];
				npc++;
			}
			break;
		case 3: /* DSZ */
			dur += timing[TIME_ISZ];
			u = cr(t);
			cw(t, --u);
			if (u == 0) {
				dur += timing[TIME_ISZ_SKP];
				npc++;
			}
			break;
		}
		break;
	case 1:	/* LDA */
		dur += timing[TIME_LDA];
		rpd = &acc[(ci >> 11) & 0x3];
		t = EA();
		*rpd = cr(t);
		break;
	case 2:	/* STA */
		dur += timing[TIME_STA];
		rpd = &acc[(ci >> 11) & 0x3];
		t = EA();
		cw(t, *rpd);
		break;
	case 3:
		Insn_IO();
		break;
	default:
		Insn_ALU();
		break;
	}
	pc = npc & 0x7fff;
}
