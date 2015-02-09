/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
#include <sys/endian.h>
#if 0
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <lagud.h>
#include <termios.h>
#include <sys/select.h>
#endif

#include "rc3600_io.h"

#define DKPSIZE		(512 * 12 * 2 * 203)

struct dkp_drive {
	const char	*file;
	uint16_t	cyl;
	u_char dkp[DKPSIZE];
};

struct dkp {
	struct dkp_drive drive[4];
	struct dkp_drive *dp;
	uint16_t ra, rb, rc, stat, dr;
};

#if 0
static void
dkp_init(int unit, int drive, const char *filename)
{
	FILE *f;
	struct dkp *dkp;

	dkp = iodevs[unit].priv;
	if (dkp == NULL) {
		dkp = calloc(sizeof *dkp, 1);
		if (dkp == NULL)
			err(1, "malloc failed");
		iodevs[unit].priv = dkp;
		printf("UNIT %d = dkp \"%s\"\n", unit, filename);
		dkp->dp = &dkp->drive[0];
	}
	dkp->drive[drive].file = filename;
	dkp->stat = 0x0040;
	f = fopen(dkp->drive[drive].file, "r");
	if (f == NULL)
		err(1, "cannot open \"%s\"", dkp->drive[drive].file);
	if (DKPSIZE != fread(dkp->drive[drive].dkp, 1, DKPSIZE, f))
		warn("read error \"%s\"", dkp->drive[drive].file);
	fclose(f);
}

static void
dkp_save(int unit, int drive, const char *filename)
{
	FILE *f;
	struct dkp *dkp;

	dkp = iodevs[unit].priv;
	f = fopen(filename, "w");
	if (f == NULL)
		err(1, "cannot open %s", dkp->drive[drive].file);
	if (DKPSIZE != fwrite(dkp->drive[drive].dkp, 1, DKPSIZE, f))
		err(1, "read error %s", dkp->drive[drive].file);
	fclose(f);
}
#endif

static void
dev_dkp_read(void *arg1, int arg2 __unused)
{
	struct iodev *io;
	struct dkp *dkp;
	int i, s, h, c, n;
	uint16_t u;

	io = arg1;
	dkp = io->priv;

	s = (dkp->rc >> 4) & 0xf;
	if (s > 11) {
		dkp->rc += 0x40;
		s = (dkp->rc >> 4) & 0xf;
	}
	h = (dkp->rc >> 8) & 0x1f;
	c = dkp->dp->cyl;
	n = 16 - (dkp->rc & 0xf);
	ioprint("DKP: READ A: %04x B: %04x C: %04x n %d c %d h %d s %d",
	    dkp->ra, dkp->rb, dkp->rc, n, c, h, s);
	for (i = 0; i < 256; i++) {
		u = be16dec(&dkp->dp->dkp[512 * (s + h * 12 + c * 24) + i * 2]);
		cw(i + dkp->rb, u);
	}
	dkp->rb += 256;
	dkp->rc += 0x11;
	s = (dkp->rc >> 4) & 0xf;
	if (dkp->rc & 0xf) {
		timeout(10000, dev_dkp_read, arg1, 0);
		return;
	}
	if (!(dkp->rc & 0xf))
		dkp->rc -= 0x10; /* correct for overflow from bottom 4 bits */
	dkp->stat |= 0x8000;
	dkp->stat |= 0x4000 >> dkp->dr;
	dev_irq(arg1, 0);
}

static void
dev_dkp(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev)
{
	struct dkp *dkp;
	int s, h, c, i, n;

	dkp = iodev->priv;
	switch (IO_OPER(ioi)) {
	case DOC:
		ioprint("C = %04x", *reg);
		dkp->rc = *reg;
		dkp->dr = *reg >> 14;
		ioprint("drive = %d", dkp->dr);
		dkp->dp = &dkp->drive[dkp->dr];
		break;
	case DIC:
		*reg = dkp->rc;
		break;
	case DOB:
		ioprint("B = %04x", *reg);
		dkp->rb = *reg;
		break;
	case DIB:
		*reg = dkp->rb;
		break;
	case DIA:
		*reg = dkp->stat;
		break;
	case NIO:
		dkp->ra = 0;
		dkp->rb = 0;
		dkp->rc = 0;
		break;
	case DOA:
		ioprint("A = %04x", *reg);
		dkp->ra = *reg;
		dkp->stat &= ~(*reg & 0xf800);
		irq_lower(iodev);
		break;
	default:
		return;
	}
	switch (IO_ACTION(ioi)) {
	case IO_START:
		break;
	case IO_PULSE:
		break;
	case IO_CLEAR:
		irq_lower(iodev);
		return;
	default:
		return;
	}
	dkp->stat &= ~(0x3f);
	s = (dkp->rc >> 4) & 0xf;
	h = (dkp->rc >> 8) & 0x1f;
	c = dkp->dp->cyl;
	n = 16 - (dkp->rc & 0xf);
	switch (dkp->ra & 0x0300) {
	case 0x0000:	/* READ */
		ioprint("READ A: %04x B: %04x C: %04x n %d c %d h %d s %d",
		    dkp->ra, dkp->rb, dkp->rc, n, c, h, s);
		iodev->busy = 1;
		timeout(100000, dev_dkp_read, iodev, 0);
		break;
	case 0x0100:	/* WRITE */
		ioprint("WRITE A: %04x B: %04x C: %04x n %d c %d h %d s %d",
		    dkp->ra, dkp->rb, dkp->rc, n, c, h, s);
		for (i = 0; i < 256 * n; i++) {
			be16enc(
			    &dkp->dp->dkp[512 * (s + h * 12 + c * 24) + i * 2],
			    cr(i + dkp->rb));
		}
		dkp->rb += n * 256 + 2;
		dkp->stat |= 0x8000;
		dkp->stat |= 0x4000 >> dkp->dr;
		iodev->busy = 1;
		timeout(100000, dev_irq, iodev, 0);
		break;
	case 0x0200:	/* SEEK */
		iodev->busy = 1;
		dkp->dp->cyl = dkp->ra & 0xff;
		ioprint("SEEK dr=%d cyl=%d", dkp->dr, dkp->dp->cyl);
		dkp->stat |= 0x4000 >> dkp->dr;
		timeout(10000, dev_irq, iodev, 0);
		break;
	case 0x0300:	/* RECAL */
		iodev->busy = 1;
		dkp->dp->cyl = 0;
		dkp->rc = 0;
		dkp->stat |= 0x4000 >> dkp->dr;
		ioprint("RECAL dr=%d cyl=%d", dkp->dr, dkp->dp->cyl);
		timeout(100000, dev_irq, iodev, 0);
		break;
	}
	return;
}

int
config_dkp(char **ap)
{
	struct dkp *dkp;
	u_int unit;
	FILE *f;

	if (strcmp(ap[0], "dkp"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[59].func = dev_dkp;
		dkp = calloc(sizeof *dkp, 1);
		if (dkp == NULL)
			err(1, "malloc failed");
		iodevs[59].priv = dkp;
		iodevs[59].imask = 10;
		dkp->dp = &dkp->drive[0];
		return (1);
	}
	unit = strtoul(ap[1], NULL, 0);
	dkp = iodevs[59].priv;
	if (!strcmp(ap[2], "load")) {
		dkp->drive[unit].file = ap[3];
		dkp->stat = 0x0040;
		f = fopen(dkp->drive[unit].file, "r");
		if (f == NULL)
			err(1, "cannot open \"%s\"", dkp->drive[unit].file);
		if (DKPSIZE != fread(dkp->drive[unit].dkp, 1, DKPSIZE, f))
			warn("read error \"%s\"", dkp->drive[unit].file);
		fclose(f);
		return (1);
	}
	if (!strcmp(ap[2], "save")) {
		f = fopen(ap[3], "w");
		if (f == NULL)
			err(1, "cannot open \"%s\"", dkp->drive[unit].file);
		if (DKPSIZE != fwrite(dkp->drive[unit].dkp, 1, DKPSIZE, f))
			warn("write error \"%s\"", dkp->drive[unit].file);
		fclose(f);
		return (1);
	}
	return (1);
}
