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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/endian.h>
#include <sys/queue.h>
#include <lagud.h>
#include <termios.h>
#include <sys/select.h>

#include "rc3600_io.h"

/* FLOPPY =========================================================== */

#define FLOPPYSIZE	(128 * 26 * 77)

struct floppy {
	int		nbr;
	const char	*file;
	u_char fd0[FLOPPYSIZE];
	u_char fd_sect[128];
	int fd_t, fd_s, fd_c;
	uint16_t fd_stat, fd_doc;
};

static void
dev_floppy(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
	struct floppy *fd;
	enum {FD_READ, FD_WRITE, FD_NOTHING}	action;

	ioprint("FLOPPY ");
	fd = iodev->priv;
	action = FD_NOTHING;
	switch (IO_OPER(ioi)) {
	case NIO:
		break;
	case DIA:
		*reg = fd->fd_stat;
		break;
	case DIB:
		*reg = fd->fd_sect[fd->fd_c++];
		ioprint("DIB %02x", *reg);
		fd->fd_stat = 0x0000;
		break;
	case DIC:
		*reg = 0xfb;
		break;
	case DOA:
		ioprint("DOA = %04x", *reg);
		switch((*reg >> 8) & 3) {
		case 0:
			fd->fd_s = (*reg) & 0xff;
			fd->fd_c = 0;
			action = FD_READ;
			break;
		case 1:
			fd->fd_s = (*reg) & 0xff;
			fd->fd_c = 0;
			action = FD_WRITE;
			break;
		case 2: /* RECAL */
			fd->fd_t = 0;
			fd->fd_s = 1;
			fd->fd_c = 0;
			break;
		case 3:
			fd->fd_t = (*reg) & 0xff;
			fd->fd_c = 0;
			break;
		default:
			printf("DOA = %04x\n", *reg);
			exit (0);
		}
		break;
	case DOB:
		fd->fd_sect[fd->fd_c] = *reg;
		fd->fd_c++;
		fd->fd_stat = 0x0000;
		break;
	case DOC:
		fd->fd_doc = *reg;
		ioprint("DOC %04x", *reg);
		break;
	default:
		return;
	}
	switch (IO_ACTION(ioi)) {
	case IO_START:
		if (action == FD_READ)
			memcpy(fd->fd_sect, &fd->fd0[(fd->fd_t * 26 + (fd->fd_s - 1)) * 128], 128);
		else if (action == FD_WRITE)
			memcpy(&fd->fd0[(fd->fd_t * 26 + (fd->fd_s - 1)) * 128], fd->fd_sect, 128);
		iodev->busy = 1;
		fd->fd_stat = 0x0000;
		timeout(100000, dev_irq, iodev, 0);
		break;
	case IO_CLEAR:
		irq_lower(iodev);
		break;
	default:
		return;
	}
}


static struct floppy fd[2];

int
config_floppy(char **ap)
{

	if (strcmp(ap[0], "floppy"))
		return (0);
	if (!strcmp(ap[1], "config")) {
                iodevs[49].func = dev_floppy;
                iodevs[49].imask = 8;
                iodevs[49].priv = &fd[0];
                iodevs[52].func = dev_floppy;
                iodevs[52].imask = 9;
                iodevs[52].priv = &fd[1];
		return (1);
	}
	if (!strcmp(ap[1], "load")) {
		FILE *f;
	
		f = fopen(ap[2], "r");
		if (f == NULL)
			err(1, "Cannot open: %s", ap[2]);
		if (77 * 26 != fread(fd[0].fd0, 128, 77 * 26, f))
			err(1, "read error: %s", ap[2]);
		fclose(f);
		return (1);
	}
	return (0);
}
