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
	int			unit_i;
	int			mask_i;
	int			unit_o;
	int			mask_o;
	const char		*dev;
	int			fd;

	int			ichar;
};

static struct tty2 tty2[] = {
	{ 010, 14, 011,  14, "/dev/nmdm0A", -1, -1},
	{ 050, 13, 051,  13, "/dev/nmdm1A", -1, -1},
};

static unsigned n_tty2 = sizeof(tty2) / sizeof(tty2[0]);

static void
ievt(void *priv, int x)
{
	struct iodev *iodev = priv;
	struct tty2 *t2 = iodev->priv;
	uint8_t u;
	int i;

	(void)x;
	if (t2->ichar == -1 && iodev->busy) {
		i = read(t2->fd, &u, 1);
		if (i == 1) {
			t2->ichar = u;
			dev_irq(iodev, 0);
			iodev->done = 1;
			ioprint("RD %d %02x", i, t2->ichar);
		} else {
			timeout(1000000, ievt, priv, 0);
		}
	}
}

static void
dev_tty2i(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
	struct tty2 *t2 = iodev->priv;

	(void)reg;
	(void)t2;

	switch (IO_OPER(ioi)) {
	case DIA:
		ioprint("GOT %02x", t2->ichar);
		*reg = t2->ichar;
		t2->ichar = -1;
		irq_lower(iodev);
		break;
	case SKPDN:
		if (iodev->done)
	default:
		break;
	}
	switch (IO_ACTION(ioi)) {
	case IO_CLEAR:
		irq_lower(iodev);
		break;
	case IO_START:
		iodev->busy = 1;
		timeout(1000000, ievt, iodev, 0);
		break;
	default:
		break;
	}
	return;
}

static void
dev_tty2o(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
	static uint8_t u = 0x21;
	struct tty2 *t2 = iodev->priv;

// fprintf(stderr, "TTYO %04x %04x\n\r", IO_OPER(ioi), IO_ACTION(ioi));
	switch (IO_OPER(ioi)) {
	case NIO:
		break;
	case DOA:
		u = *reg & 0xff;
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
		write(t2->fd, &u, 1);
		timeout(11 * (NSEC / 9600), dev_irq, iodev, 0);
		break;
	default:
		break;
	}
	return;
}

int
config_tty2(char **ap)
{
	unsigned n;
	int fd;
	struct tty2 *t;
	struct termios tt;

	if (strcmp(ap[0], "tty2"))
		return (0);
	ap++;
	if (*ap == NULL)
		return (0);

	/* Get unit number */
	n = atoi(ap[0]);
	assert(n < n_tty2);
	ap++;
	if (*ap == NULL)
		return (0);

	t = &tty2[n];

	if (!strcmp(ap[0], "config")) {
		fd = open(t->dev, O_RDWR, 0);
		if (fd < 0) {
			printf("Cannot open %s: %s\n", t->dev, strerror(errno));
			return (1);
		}
		if (t->fd >= 0)
			assert(close(t->fd) == 0);
		t->fd = fd;
		t->ichar = -1;

		assert(tcgetattr(t->fd, &tt) == 0);
		cfmakeraw(&tt);
		cfsetspeed(&tt, B9600);
		tt.c_cflag |= CLOCAL;
		tt.c_cc[VMIN] = 0;
		tt.c_cc[VTIME] = 0;
		assert(tcsetattr(t->fd, TCSAFLUSH, &tt) == 0);

		iodevs[t->unit_i].func = dev_tty2i;
		iodevs[t->unit_i].priv = t;
		iodevs[t->unit_i].imask = t->mask_i;
		iodevs[t->unit_o].func = dev_tty2o;
		iodevs[t->unit_o].imask = t->mask_o;
		iodevs[t->unit_o].priv = t;

		return (1);
	}
	return (0);
}
