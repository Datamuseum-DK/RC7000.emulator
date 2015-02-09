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

/* RTC ============================================================== */

static int rtc_dly = NSEC / 50;

static void
dev_rtc_tick(void *arg1, int arg2 __unused)
{
	struct iodev *io = arg1;

	timeout(rtc_dly, dev_rtc_tick, io, 0);
	dev_irq(arg1, arg2);
}

static void
dev_rtc(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev)
{

// fprintf(stderr, "RTC  %04x %04x %g [s]\n\r", IO_OPER(ioi), IO_ACTION(ioi), rtc_dly * 1e-9);
	switch (IO_OPER(ioi)) {
	case DOA:
		switch ((*reg) & 3) {
		case 0:	rtc_dly = NSEC / 50;	break;
		case 1:	rtc_dly = NSEC / 10;	break;
		case 2:	rtc_dly = NSEC / 100;	break;
		case 3:	rtc_dly = NSEC / 1000;	break;
		}
		ioprint("Rate %g [s]", rtc_dly * 1e-9);
		break;
	case NIO:
		break;
	default:
		return;
	}
	switch (IO_ACTION(ioi)) {
	case IO_CLEAR:
		irq_lower(iodev);
		break;
	case IO_START:
		irq_lower(iodev);
		iodev->busy = 1;
		break;
	default:
		break;
	}
	return;
}

int
config_rtc(char **ap)
{

	if (strcmp(ap[0], "rtc"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[12].func = dev_rtc;
		iodevs[12].imask = 13;
		timeout(rtc_dly, dev_rtc_tick, &iodevs[12], 0);
		return (1);
	}
	return (0);
}
