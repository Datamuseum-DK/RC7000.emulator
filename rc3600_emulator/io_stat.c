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
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "rc3600_io.h"

static uint64_t Lins, Lrd, Lwr;

/* RTC ============================================================== */

static void
dev_stat(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev __unused)
{

fprintf(stderr, "STAT  %04x %04x\n\r", IO_OPER(ioi), IO_ACTION(ioi));
	switch (IO_OPER(ioi)) {
	case DIA:
		Lins = NINS;
		Lrd = NRD;
		Lwr = NWR;
		break;
	case DOA:
		fprintf(stderr, "INS %ju RD %ju WR %ju\r\n",
		    (uintmax_t)NINS,
		    (uintmax_t)NRD,
		    (uintmax_t)NWR);
		fprintf(stderr, "ins %ju rd %ju wr %ju\r\n",
		    (uintmax_t)NINS-Lins,
		    (uintmax_t)NRD-Lrd,
		    (uintmax_t)NWR-Lwr);
		break;
	default:
		return;
	}
	switch (IO_ACTION(ioi)) {
	case IO_CLEAR:
		break;
	case IO_START:
		break;
	default:
		break;
	}
	return;
}

int
config_stat(char **ap)
{

	if (strcmp(ap[0], "stat"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[62].func = dev_stat;
		iodevs[62].imask = 0x0008;
		return (1);
	}
	return (0);
}
