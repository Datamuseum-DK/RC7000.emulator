#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/endian.h>

#include "lagud.h"

static const char *alu[8] = {"COM", "NEG", "MOV", "INC", "ADC", "SUB", "ADD", "AND" };
static const char *carry[4] = {" ", "Z", "O", "C" };
static const char *shift[4] = {" ", "L", "R", "S" };
static const char *skip[8] = {"    ", ",SKP", ",SZC", ",SNC", ",SZR", ",SNR", ",SEZ", ",SBN" };
static const char *hash[2] = {"   ", " # " };
static const char *jjid[4] = {"JMP", "JSR", "ISZ", "DSZ" };
static const char *at[2] = {"   ", " @ " };
static const char *ldst[4] = {NULL, "LDA", "STA", NULL};
static const char *io[8] = {"NIO", "DIA", "DOA", "DIB", "DOB", "DIC", "DOC", "SKP" };
static const char *test[4] = {"BN", "BZ", "DN", "DZ" };
static const char *func[4] = {"  ", "S ", "C ", "P " };

static const char *iodev[64] = {
	"00",	"01", 	"02", 	"03", 	"04", 	"05", 	"06", 	"07",
	"TTYI",	"TTYO",	"PTR", 	"PTP", 	"RTC", 	"0d", 	"0e", 	"0f",
	"10",	"11", 	"12", 	"13", 	"14", 	"15", 	"16", 	"17",
	"18",	"19", 	"1a", 	"1b", 	"1c", 	"1d", 	"1e", 	"1f",
	"20",	"21", 	"22", 	"23", 	"24", 	"25", 	"26", 	"27",
	"28",	"29", 	"AMX", 	"2b", 	"2c", 	"2d", 	"2e", 	"2f",
	"30",	"FD0", 	"32", 	"33", 	"FD1", 	"35", 	"36", 	"37",
	"38",	"39", 	"3a", 	"DKP", 	"3c", 	"3d", 	"3e", 	"CPU",
};

struct magic {
	u_int		n;
	const char	*s;
};

#include "magic.c"

static const char *PZ[256] = {
/* 00 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 10 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 20 */ "CUR",	   NULL,     NULL,     NULL,	 NULL,	   "TABLE",  "TOPTA", NULL,
	 NULL,	   NULL,     "PFIRS",  NULL,	 "RUNNI",  ".0",     "EXIT",  "EFIRS",
/* 30 */ "FFIRS",  "DELAY",  NULL,     NULL,	 "AREAP",  "AFIRS",  "FREQ",  "MASK",
	 "CORES",  "PROGR",  NULL,     NULL,     "RTIME",  "RTIME+1","POWIN",  NULL,
/* 40 */ "CPUTY",  ".32768", ".16384", ".8192",  ".4096",  ".2048",  ".1024",  ".512",
	 ".256",   ".128",   ".64",    ".32",    ".16",    ".8",     ".4",     ".2",
/* 50 */ ".1",     ".3",     ".5",     ".6",	 ".7",	   ".9",     ".10",    ".12",
	 ".13",    ".15",    ".24",    ".25",	 ".40",	   ".48",    ".56",    ".60",
/* 60 */	".63",	".120",	".127",	".255",	".M3",	".M4",	".M16",	".M256",
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 70 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 80 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 90 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* a0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* b0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* c0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* d0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* e0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* f0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
};

static void
displ(char *buf, u_int u, int *dsp, int domus)
{
	int i, d;

	d = (u >> 8) & 3;
	i = u & 0x3ff;

	if (domus && d == 0 && PZ[i] != NULL) {
		strcpy(buf, PZ[i]);
		return;
	}

	i = u & 0xff;
	if (d != 0 && i > 0x7f) {
		i -= 256;
		sprintf(buf, "-%02x,%d ", -i, d);
	} else {
		sprintf(buf, "+%02x,%d ", i, d);
	}
	if (d == 1 && dsp != NULL)
		*dsp = i;
}

void
LagudDisass(char *buf, u_int u, int *d, int domus)
{
	int i;

	struct magic const *sp;

	if (d != NULL)
		*d = -99999;
	for (sp = magic; domus && sp->s != NULL; sp++) {
		if (u == sp->n) {
			strcpy(buf, sp->s);
			return;
		}
	}
	if (u & 0x8000) {
		strcpy(buf, alu[(u >> 8) & 7]);
		strcat(buf, carry[(u >> 4) & 3]);
		strcat(buf, shift[(u >> 6) & 3]);
		strcat(buf, hash[(u >> 3) & 1]);
		sprintf(buf + strlen(buf), "%d,%d",
		    (u >> 13) & 3, (u >> 11) & 3);
		strcat(buf, skip[u & 7]);
	} else if ((u & 0xe000) == 0x0000) {
		strcpy(buf, jjid[(u >> 11) & 3]);
		strcat(buf, at[(u >> 10) & 1]);
		displ(buf + strlen(buf), u, d, domus);
		strcat(buf, "  ");
	} else if ((u & 0xe000) == 0x6000) {
		i = u & 0x3f;
		strcpy(buf, io[(u >> 8) & 7]);
		if (((u >> 8) & 7) == 7) {
			strcat(buf, test[(u >> 6) & 3]);
			sprintf(buf + strlen(buf), "   %s     ", iodev[i]);
		} else {
			strcat(buf, func[(u >> 6) & 3]);
			sprintf(buf + strlen(buf), " %d,%s     ",
			   (u >> 11) & 3, iodev[i]);
		}
	} else {
		i = u & 0xff;
		if (i > 0x7f)
			i -= 256;
		strcpy(buf, ldst[(u >> 13) & 3]);
		strcat(buf, at[(u >> 10) & 1]);
		sprintf(buf + strlen(buf), "%d,",
		    (u >> 11) & 3);
		displ(buf + strlen(buf), u, d, domus);
	}
}

#ifdef LAGUD_MAIN
int
main(int argc __unused, char **argv __unused)
{
	char buf[40], ascii[2];
	uint16_t u, v, a;
	int i, d;

	for(a = 0; ; a++) {
		i = read(0, &v, sizeof v);
		if (i != sizeof v)
			break;
		i = v & 0xff;
		if (i > 0x20 && i < 0x7f)
			ascii[0] = i;
		else
			ascii[0] = ' ';
		i = v >> 8;
		if (i > 0x20 && i < 0x7f)
			ascii[1] = i;
		else
			ascii[1] = ' ';
		u = be16dec(&v);
		LagudDisass(buf, u, &d, 0);
		printf("%05d %04x %c%c %04x  %s  ", a, a, ascii[1], ascii[0], u, buf);
		if (d != 0)
			printf("->%04x", a + d);
		printf("\n");
	}
	return(0);
}
#endif
