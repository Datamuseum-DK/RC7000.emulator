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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "rc3600_io.h"

static int ttyi_char = -1;
static struct iodev *ttyi_iodev;

static int tty_speed = 9600;

#if 0
/*
 * TTY boot loader.
 * from "How to use the Nova Computers", 015-000009-08 p. 2-55.
 */
uint16_t ttyload[13] = {
	0126440, /* GET:   SUBO	  1,1	; Clear AC1, Carry	           */
	0063610, /*	   SKPDN  TTI				           */
	0000777, /*	   JMP	  .-1	; Wait for Done		           */
	0060510, /*	   DIAS	  0,TTI	; Read into AC0 and restart reader */
	0127100, /*	   ADDL   1,1   ; Shift AC1 left 4 places	   */
	0127100, /*	   ADDL   1,1					   */
	0107003, /*	   ADD	  0,1,SNC ; Add in new word		   */
	0000772, /*	   JMP	  GET+1	; Full word not assembled yet	   */
	0001400, /*	   JMP	  0,3	; Got full word, exit		   */
	0060100, /* BSTRP: NIOS   TTI	; Enter here, start reader	   */
	0004766, /*	   JSR	  GET	; Get a word			   */
	0044402, /*	   STA	  1,.+2	; Store it to execute it	   */
	0004764  /*	   JSR	  GET	; Get another word		   */
		 /*	   ; This will contain an STA (first STA 1,+1)	   */
		 /*	   ; This will contain JMP .-4                     */
};
#endif

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

int
config_tty(char **ap)
{

	if (strcmp(ap[0], "tty"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		ttyi_iodev = &iodevs[8];
		iodevs[8].func = dev_ttyi;
		iodevs[8].imask = 7;
		iodevs[9].func = dev_ttyo;
		iodevs[9].imask = 6;
		return (1);
	}
	if (!strcmp(ap[1], "speed")) {
		if (ap[2] != NULL)
			tty_speed = strtoul(ap[2], NULL, 0);
		else
			tty_speed = 9600;
		return (1);
	}
	return (0);
}
