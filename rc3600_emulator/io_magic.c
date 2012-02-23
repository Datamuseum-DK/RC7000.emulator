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

static FILE *magic_i, *magic_o;

static int
dev_magic(uint16_t ioi, uint16_t *reg __unused, struct iodev *iodev __unused)
{
	char cbuf[100];
	u_int i;
	int j;
	uint16_t u, r;

	r = *reg;
	switch (ioi) {
	case 0x6047:	/* NIOS 0,7 */
		verbose = 1;
		return (0);
	case 0x6207:	/* DOA	0,7 */
		u = 0;
		for (i = 0; i < sizeof cbuf; i++) {
			if (!(i & 1)) {
				u = CR(r++);
				cbuf[i] = u >> 8;
			} else {
				cbuf[i] = u & 0xff;
			}
			if (cbuf[i] == 0)
				break;
		}
		fprintf(stderr,"cbuf: %d<%s>\r\n", cbuf[0], cbuf + 1);
		switch(cbuf[0]) {
		case 1:
			if (magic_o != NULL)
				fclose(magic_o);
			magic_o = fopen(cbuf + 1, "w");
			if (magic_o == NULL) {
				warn("Could not open %s -- dropping",
				     cbuf + 1);
				magic_o = NULL;
			}
			break;
		case 2:
			if (magic_o != NULL) {
				fclose(magic_o);
				magic_o = NULL;
			}
			break;
		case 3:
			if (magic_i != NULL)
				fclose(magic_i);
			magic_i = fopen(cbuf + 1, "r");
			if (magic_i == NULL) {
				warn("Could not open %s -- dropping",
				     cbuf + 1);
				magic_i = NULL;
			}
			break;
		case 4:
			if (magic_i != NULL) {
				fclose(magic_i);
				magic_i = NULL;
			}
			break;
		default:
			return (1);
		}
		break;
	case 0x6307:	/* DIB	0,7 */
		if (magic_i != NULL) {
			j = fgetc(magic_i);
			if (j == EOF)
				*reg = 0xffff;
			else
				*reg = j & 0xff;
		} else {
				*reg = 0xffff;
		}
		return (0);
	case 0x6407:	/* DOB	0,7 */
	case 0x6c07:	/* DOB	1,7 */
		if (magic_o != NULL)
			fputc(r, magic_o);
		return (0);
	case 0x6607:	/* DOC	0,7 */
		if (magic_o != NULL) {
			fclose(magic_o);
			magic_o = NULL;
		}
		if (magic_i != NULL) {
			fclose(magic_i);
			magic_i = NULL;
		}
		return (0);
	default:
		return (1);
	}
	return (0);
}
