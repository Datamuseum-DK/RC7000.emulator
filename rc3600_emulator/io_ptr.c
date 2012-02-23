/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */

#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "err.h"
#include "sys/types.h"
#include "rc3600_io.h"

static FILE *ptr_file;
static int ptr_state = 2, ptr_count;

static char *ptr_filename;

static int
dev_ptr_state(void)
{
	int i;

	switch(ptr_state) {
	case 0:
		ptr_state = 1;
		ptr_count = 511;
		i = 0;
		break;
	case 1:
		if (--ptr_count == 0)
			ptr_state = 2;
		i = 0;
		break;
	case 2:
		fprintf(stderr, "PTR open \"%s\"\r\n", ptr_filename);
		ptr_file = fopen(ptr_filename, "r");
		assert(ptr_file != NULL);
		ptr_state = 3;
		/* FALLTHROUGH */
	case 3:
		i = getc(ptr_file);
		if (i == -1) {
			i = 0x19;
			ptr_count = 512;
			ptr_state = 5;
			fclose(ptr_file);
			fprintf(stderr, "PTR close\r\n");
			ptr_file = NULL;
		}
		if (0) {
			if (i == '\n') {
				i = '\r';
				ptr_state = 4;
			}
		}
		break;
	case 4:
		i = 0x0a;
		ptr_state = 3;
		break;
	case 5:
		i = 0;
		if (--ptr_count == 0)
			ptr_state = 0;
		break;
	default:
		errx(1, "wrong ptr_state %d", ptr_state);
	}
	if (1) {
		if (i >= ' ' && i <= '~')
			ioprint("PTR(%c)", i);
		else
			ioprint("PTR(%02x)", i);
	}
	return (i);
}

static void
dev_ptr(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
if (0)
printf("PTR %x s %d b %d d %d\r\n", ioi, ptr_state, iodev->busy, iodev->done);
	ioprint("PTR ");
	switch (IO_OPER(ioi)) {
	case NIO:
		break;
	case DIA:
		*reg = dev_ptr_state();
		break;
	default:
		return;
	}
	switch (IO_ACTION(ioi)) {
	case IO_CLEAR:
		irq_lower(iodev);
		break;
	case IO_START:
		iodev->busy = 1;
		timeout(NSEC / 1000, dev_irq, iodev, 0);
		break;
	}
	return;
}

int
config_ptr(char **ap)
{

	if (strcmp(ap[0], "ptr"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		iodevs[10].func = dev_ptr;
		iodevs[10].imask = 2;
		ptr_filename = strdup("_.ptr");
		return (1);
	}
	if (!strcmp(ap[1], "file") && ap[2] != NULL) {
		free(ptr_filename);
		ptr_filename = strdup(ap[2]);
		ptr_state = 2;
		return (1);
	}
	return (0);
}
