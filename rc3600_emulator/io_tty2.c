/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include "rc3600_io.h"

struct tty2 {
	int			io_i;
	int			mask_i;
	int			io_o;
	int			mask_o;
	const char		*dev;
	int			fd;
};

static struct tty2 tty2[] = {
	{ 010, 14, 011,  14, "/dev/nmdm0A", -1 },
	{ 050, 13, 051,  13, "/dev/nmdm1A", -1 },
};

static unsigned n_tty2 = sizeof(tty2) / sizeof(tty2[0]);

#if 0

int
TTYI_Input(int chr)
{

	if (ttyi_char != -1 || ttyi_iodev == NULL)
		return (0);
	ttyi_char = chr;
	ttyi_iodev->busy = 1;
	dev_irq(ttyi_iodev, 0);
	return (1);
}

static void
dev_ttyi(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{

// fprintf(stderr, "TTYI %04x %04x\n\r", IO_OPER(ioi), IO_ACTION(ioi));
	switch (IO_OPER(ioi)) {
	case NIO:
		break;
	case DIA:
		ioprint(" %02x", ttyi_char);
		*reg = ttyi_char;
		ttyi_char = -1;
		irq_lower(iodev);
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
		ttyi_iodev->busy = 1;
		if (ttyi_char != -1)
			dev_irq(iodev, 0);
		break;
	default:
		break;
	}
	return;
}

/* TTYO ============================================================= */

static void
dev_ttyo(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
	static char c;

// fprintf(stderr, "TTYO %04x %04x\n\r", IO_OPER(ioi), IO_ACTION(ioi));
	switch (IO_OPER(ioi)) {
	case NIO:
		break;
	case DOA:
		c = *reg & 0x7f;
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
		if (c == '\n' || c == '\r') {
			fprintf(stderr, "%c", c);
		} else if (c < ' ' || c > '~') {
			c = ' ';
		} else {
			fprintf(stderr, "%c", c);
		}
		fflush(stderr);
		timeout(11 * (NSEC / tty_speed), dev_irq, iodev, 0);
		break;
	default:
		break;
	}
	return;
}

#endif

int
config_tty2(char **ap)
{
	unsigned n;
	struct tty2 *t;
	struct termios tt;

	if (strcmp(ap[0], "tty2"))
		return (0);
	ap++;
	if (*ap == NULL)
		return (0);

	/* Get unit number */
	n = atoi(ap[0] + 1);
	assert(n < n_tty2);
	ap++;
	if (*ap == NULL)
		return (0);

	t = &tty2[n];

	if (!strcmp(ap[0], "config")) {
		// iodevs[t->io_i].func = dev_ttyi;
		iodevs[t->io_i].imask = t->mask_i;
		// iodevs[t->io_o].func = dev_ttyo;
		iodevs[t->io_o].imask = t->mask_o;
		if (t->fd >= 0)
			assert(close(t->fd) == 0);
		t->fd = open(t->dev, O_RDWR, 0);
		if (t->fd < 0) {
			printf("Cannot open %s: %s\n", t->dev, strerror(errno));
			return (1);
		}
		assert(tcgetattr(t->fd, &tt) == 0);
		cfmakeraw(&tt);
		cfsetspeed(&tt, B9600);
		tt.c_cflag |= CLOCAL;
		tt.c_cc[VMIN] = 0;
		tt.c_cc[VTIME] = 10;
		assert(tcsetattr(t->fd, TCSAFLUSH, &tt) == 0);
		return (1);
	}
	return (0);
}
