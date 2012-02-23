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
#include <sys/types.h>

#include "rc3600_io.h"

static FILE *lpt;

static void
dev_lpt(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
	int i;

	if (lpt == NULL)
		lpt = fopen("/tmp/_.lpt", "a");

	switch (ioi) {
	case NIOC:
		irq_lower(iodev);
		break;
	case DIA:
		*reg = 0x0;
		break;
	case DOAS:
		if (*reg < 128)
			fprintf(lpt, "%c", *reg);
		else {
			i = *reg >> 8;
			while (--i)
				fprintf(lpt, "\n");
		}
		fflush(lpt);
		iodev->busy = 1;
		timeout(70, dev_irq, iodev, 0);
		break;
	case SKPBN:
		break;
	case SKPDN:
	case SKPBZ:
		npc++;
		break;
	default:
		return;
	}
}

int
config_lpt(char **ap)
{

	if (strcmp(ap[0], "lpt"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[15].func = dev_lpt;
		iodevs[15].imask = 13;
		return (1);
	}
	return (0);
}
