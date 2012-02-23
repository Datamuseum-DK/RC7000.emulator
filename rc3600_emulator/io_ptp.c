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

static FILE *ptp_file;

static void
dev_ptp(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev)
{

	if (ptp_file == NULL)
		ptp_file = fopen("_.ptp", "a");
	switch (ioi) {
	case NIOC:
		irq_lower(iodev);
		fflush(ptp_file);
		return;
	case DOAS:
		fputc(*reg, ptp_file);
		iodev->busy = 1;
		dev_irq(iodev, 0);
		return;
	case DIA:
		*reg = 0;
		return;
	default:
		return;
	}
}

int
config_ptp(char **ap)
{

	if (strcmp(ap[0], "ptp"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[11].func = dev_ptp;
		iodevs[11].imask = 1;
		return (1);
	}
	return (0);
}
