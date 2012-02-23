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

#include "rc3600_io.h"

#define FLPSIZE		(128 * 26 * 1 * 77)

struct flp_drive {
	const char	*file;
	uint16_t	cyl;
	u_char 		flp[FLPSIZE];
};

struct flp {
	struct flp_drive drive[4];
	struct flp_drive *dp;
	uint16_t ra, rb, rc, stat, dr, ns;
};

static void
rd_sect(struct flp_drive *fd, int t, int s, uint16_t a)
{
	u_char *p;
	int i;
fprintf(stderr, "rd t%d s%d a%04x\n\r", t, s, a);
	p = &fd->flp[(t * 26 + (s - 1)) * 128];
	for (i = 0; i < 64; i++)
		cw(a + i, be16dec(p + i * 2));
}

static void
dev_flp_autoload(void *arg1, int arg2 __unused)
{
	struct flp *flp;
	struct iodev *iodev;
	int i;

	iodev = arg1;
	flp = iodev->priv;
	for (i = 0; i < 26; i++)
		rd_sect(flp->drive, 0, 1 + i, 64 * i);
	iodev->busy = 1;
	flp->ns = 4 * 26;
	timeout(10000, dev_irq, iodev, 0);
}

static void
dev_flp(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev)
{
	struct flp *flp;
	uint16_t ptr;

	flp = iodev->priv;
	if (ioi == NIOS) {
		iodev->busy = 1;
		timeout(10000, dev_flp_autoload, iodev, 0);
		return;
	}
	switch (IO_OPER(ioi)) {
	case DIA:
		*reg = 0x8000;
		break;
	case DOA:
		ioprint("A = %04x", *reg);
		flp->ra = *reg;
		ptr = flp->rb + 4;
		ptr = cr(flp->rb);
{
	int i;

	fprintf(stderr, "NS READ  0x%04x -> %04x @%04x\n\r", flp->ra, flp->rb, ptr);
	for (i = 0; i < 21; i++)
		fprintf(stderr, "%04x ", cr(ptr + i));
	fprintf(stderr, "\n\r");
fflush(stdout);
}
		if (flp->ra == 4) {
	if (cr(ptr) != 0x30b1) {
		fprintf(stderr, "CMD %04x\n\r", cr(ptr));
			iodev->busy = 1;
			timeout(10000, dev_irq, iodev, 0);
		return;
	}
			if (cr(ptr + 9) > 76 || cr(ptr + 10) > 26) {
				return;
			}
			rd_sect(flp->drive, cr(ptr + 9), cr(ptr + 10), cr(ptr + 4));
			iodev->busy = 1;
			timeout(10000, dev_irq, iodev, 0);
		}
		return;
		break;
	case DOB:
		ioprint("B = %04x", *reg);
		flp->rb = *reg;
		return;
		break;
	case NIO:
		break;
	default:
		return;
	}
	switch (IO_ACTION(ioi)) {
	case IO_CLEAR:
		irq_lower(iodev);
		return;
	default:
		return;
	}

	return;
}

int
config_rc3751(char **ap)
{
	struct flp *flp;
	u_int unit;
	FILE *f;

	if (strcmp(ap[0], "rc3751"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[49].func = dev_flp;
		flp = calloc(sizeof *flp, 1);
		if (flp == NULL)
			err(1, "malloc failed");
		iodevs[49].priv = flp;
		iodevs[49].imask = 10;
		flp->dp = &flp->drive[0];
		return (1);
	}
	unit = strtoul(ap[1], NULL, 0);
	flp = iodevs[49].priv;
	if (!strcmp(ap[2], "load")) {
		flp->drive[unit].file = ap[3];
		flp->stat = 0x0040;
		f = fopen(flp->drive[unit].file, "r");
		if (f == NULL)
			err(1, "cannot open \"%s\"", flp->drive[unit].file);
		if (FLPSIZE != fread(flp->drive[unit].flp, 1, FLPSIZE, f))
			warn("read error \"%s\"", flp->drive[unit].file);
		fclose(f);
		return (1);
	}
	if (!strcmp(ap[2], "save")) {
		f = fopen(ap[3], "w");
		if (f == NULL)
			err(1, "cannot open \"%s\"", flp->drive[unit].file);
		if (FLPSIZE != fwrite(flp->drive[unit].flp, 1, FLPSIZE, f))
			warn("write error \"%s\"", flp->drive[unit].file);
		fclose(f);
		return (1);
	}
	return (1);
}
