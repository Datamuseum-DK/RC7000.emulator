/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $Id: rc3600_emul.h,v 1.3 2010/08/15 14:44:26 phk Exp $
 */

#ifndef RC3600_EMUL_H
#define RC3600_EMUL_H 1

#include <stdint.h>

enum rc3600_type {
	NOVA_2,
	NOVA_800,
	NOVA_1200,
	RC_3600,
	RC_3803
};

extern uint16_t	acc[4];			/* The accumulators */
extern uint8_t	carry;			/* Carry bit */
extern uint16_t	pc;			/* Program counter */
extern uint16_t	npc;			/* Next program counter */
extern uint16_t	ci;			/* Current instruction */

extern unsigned	*timing;		/* Timing spec, if any. */
extern unsigned	dur;			/* Duration of current instruction */

void		rc3600_exec(void);	/* Execute one instrustion */

/* Memory access, provided by implementation */
extern uint16_t	cr(uint16_t);		/* Core Read */
extern void	cw(uint16_t, uint16_t);	/* Core Write */


/* I/O device handling -----------------------------------------------*/

struct iodev;

typedef void iodev_f(uint16_t ioi, uint16_t *reg, struct iodev *iodev);

iodev_f dev_nodev;

struct iodev {
	iodev_f		*func;		/* I/O function */
	void		*priv;		/* private (instance) data */
	uint8_t		busy;		/* I/O bit */
	uint8_t		done;		/* I/O bit */
	uint8_t		unit;		/* Device number [0...63] */
	uint8_t		imask;		/* Bit in interrupt mask */
	uint8_t		ipen;		/* Interrupt Pending */
};

extern struct iodev iodevs[64];

#define	NIO		0x6000
#define	NIOS		0x6040
#define	NIOC		0x6080
#define	NIOP		0x60c0
#define	DIA		0x6100
#define	DIAS		0x6140
#define	DIAC		0x6180
#define	DIAP		0x61c0
#define	DOA		0x6200
#define	DOAS		0x6240
#define	DOAC		0x6280
#define	DOAP		0x62c0
#define	DIB		0x6300
#define	DIBS		0x6340
#define	DIBC		0x6380
#define	DIBP		0x63c0
#define	DOB		0x6400
#define	DOBS		0x6440
#define	DOBC		0x6480
#define	DOBP		0x64c0
#define	DIC		0x6500
#define	DICS		0x6540
#define	DICC		0x6580
#define	DICP		0x65c0
#define	DOC		0x6600
#define	DOCS		0x6640
#define	DOCC		0x6680
#define	DOCP		0x66c0
#define	SKPBN		0x6700 /* Not dispatched to driver */
#define	SKPBZ		0x6740 /* Not dispatched to driver */
#define	SKPDN		0x6780 /* Not dispatched to driver */
#define	SKPDZ		0x67c0 /* Not dispatched to driver */

#define IO_ACTION(x)	((x) & 0x00c0)
#define IO_START	0x0040
#define IO_CLEAR	0x0080
#define IO_PULSE	0x00c0

#endif /* RC3600_EMUL_H */
