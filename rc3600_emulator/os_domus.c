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
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "rc3600_io.h"

static FILE *fmsg;

/*
 * DOMUS idle spins by jumping to location 50 which contains "JMP 50,0"
 *
 * Trace it, and use usleep(3) to avoid eating all the cpu
 */

static void
domus_trace_50(uint16_t adr __unused, int type, u_int *next_event)
{

	if (type != 0)
		return;
	if (next_event == NULL)
		return;

	if (*next_event > 1000000) {
		usleep(*next_event / 1000);
		*next_event = 0;
	}
}

static const char *
Name(uint16_t p)
{
	static char s[6];

	s[0] = cr(p + 0) >> 8;
	s[1] = cr(p + 0) & 0xff;
	s[2] = cr(p + 1) >> 8;
	s[3] = cr(p + 1) & 0xff;
	s[4] = cr(p + 2) >> 8;
	s[5] = cr(p + 2) & 0xff;
	return (s);
}

static void
ZString(FILE *f, uint16_t t)
{
	u_int x;

	fprintf(f, "\t\"");
	while (1) {
		x = cr(t / 2);
		if (!(t & 1))
			x >>= 8;
		x &= 0xff;
		if (x >= ' ' && x <= '~')
			fprintf(f, "%c", x);
		else
			fprintf(f, "<%d>", x);
		t++;
		if (x == 0)
			break;
	}
	fprintf(f, "\"");
}

static void
domus_trace_sendm(uint16_t adr, int type, u_int *next_event __unused)
{
	uint16_t m, m0, m1, m2, m3;
	char from[6], to[6];

	if (type != 1)
		return;
	if (adr != 0xc04)
		return;
	if (fmsg == NULL)
		return;
	if (cr(50) != 50)
		return;

	m = acc[1];
	m0 = cr(m);
	m1 = cr(m + 1);
	m2 = cr(m + 2);
	m3 = cr(m + 3);
	strcpy(from, Name(cr(32) + 4));
	strcpy(to, Name(acc[2]));

	fprintf(fmsg, "%04x %04x SendM(%04x,%04x,%04x,%04x) ",
		pc, m, m0, m1, m2, m3);
	fprintf(fmsg, "%s -> %s", from, to);
	if (!strcmp(to, "S") && m2 != 0)
		ZString(fmsg, m2);
	if (!strcmp(to, "TTY") && m0 == 3)
		ZString(fmsg, m2);
	if (!strcmp(to, "DKP0")) {
		int d, c, h, s, x;

		x = m3;
		s = x % 12;	x /= 12;
		h = x % 2;	x /= 2;
		c = x % 203;	x /= 203;
		d = x;
		fprintf(fmsg, "\tdrive: %d  cyl: %d  hd: %d  sc: %d",
		    d, c, h, s);
	}
	fprintf(fmsg, "\n");
	fflush(fmsg);
}

static void
domus_trace_senda(uint16_t adr, int type, u_int *next_event __unused)
{
	uint16_t t;

	if (type != 1)
		return;
	if (adr != 0xc07)
		return;
	if (fmsg == NULL)
		return;
	if (cr(50) != 50)
		return;

	fprintf(fmsg, "%04x %04x SendA(%04x, %04x) ",
	    pc, acc[2], acc[0], acc[1]);
	t = cr(32);
	fprintf(fmsg, "%s ->", Name(t + 4));
	fprintf(fmsg, " %s\n", Name(cr(acc[2] + 4) + 4));
	fflush(fmsg);
}

int
config_domus(char **ap)
{

	if (strcmp(ap[0], "domus"))
		return (0);
	if (!strcmp(ap[1], "sleep")) {
		trace_set(50, domus_trace_50);
		return (1);
	}
	if (!strcmp(ap[1], "msg")) {
		trace_set(0xc04, domus_trace_sendm);
		trace_set(0xc07, domus_trace_senda);
		if (ap[2] != NULL) {
			fmsg = fopen(ap[2], "w");
			if (fmsg == NULL)
				warn("Could not open %s", ap[2]);
		}
		return (1);
	}
	return (0);
}
