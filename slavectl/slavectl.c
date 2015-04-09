
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <sys/stat.h>

#include "domusobj.h"
#include "rc3600.h"
#include "rc3600_emul.h"

#include "slavectl.h"

static int fo;

unsigned long	rx_count;
unsigned long	tx_count;
unsigned long	cmd_count;

static int
xfgetc(void *priv)
{
	return fgetc(priv);
}

static void
PC(uint8_t u)
{
	tx_count++;
	assert(1 == write(fo, &u, 1));
}

static void
PW(uint16_t u)
{
	// printf(">>>>%04x %06o\n", u, u);
	PC(u >> 8);
	PC(u & 0xff);
}

static uint16_t
GW(void)
{
	uint8_t c[2];
	int i;

	i = read(fo, c + 0, 1);
	assert(i == 1);
	i = read(fo, c + 1, 1);
	assert(i == 1);
	rx_count += 2;
	return ((c[0] << 8) | c[1]);
}

static void
SendCmd(uint16_t cmd, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
	uint16_t s;

	PW(cmd); s = cmd;
	PW(a0);  s += a0;
	PW(a1);  s += a1;
	PW(a2);  s += a2;
	PW(a3);  s += a3;
	PW(65536-(unsigned)s);
}

int
DoCmd(uint16_t cmd, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3,
    uint16_t *upload, uint16_t uplen, uint16_t *download, uint16_t downlen)
{
	uint16_t s;
	int i;

	cmd_count++;
	PW(cmd); s = cmd;
	PW(a0);  s += a0;
	PW(a1);  s += a1;
	PW(a2);  s += a2;
	PW(a3);  s += a3;
	PW(65536-(unsigned)s);
	printf("%04x(%04x,%04x,%04x,%04x) S|%04x",
	   cmd, a0, a1, a2, a3, 0x10000 - s);

	s = GW();
	if (s == 0)
	       return (-1);
	if (s & 0x8000) {
		/* upload */
		printf(" U[%d]", 0x10000 - s);
		i = s;
		i += uplen;
		assert(i == 0x10000);
		for (i = s; i < 0x10000; i++) {
			PW(*upload++);
			uplen--;
		}
		s = GW();
	}
	printf(" D %4d", s);
	assert(!(s & 0x8000));
	assert(s != 0);
	/* download */
	assert(s == downlen);
	for (i = 0; i < s; i++)
		*download++ = GW();
	putchar('\t');
	return (0);
}


static void
Sync(void)
{
	int i;

	printf("SYNC\t");
	i = DoCmd(0, 0, 0, 0, 0, NULL, 0, NULL, 0);
	printf(" => %d\n", i);
	assert(i == -1);
}


/**********************************************************************/

static uint16_t core[65536];

int
main(int argc, char **argv)
{
	struct domus_obj_file *dof;
	struct domus_obj_obj *doo;
	struct domus_obj_rec *dor;
	FILE *fi;
	unsigned u, a, amax;
	struct termios t;
	uint8_t c;
	int i, j;
	char buf[BUFSIZ];
	uint16_t card[80];

	setbuf(stderr, NULL);
	setbuf(stdout, NULL);
	fi = fopen("../domus/__.SLAVE", "r");

	if (argc > 1)
		fo = open(argv[1], O_RDWR);
	else
		fo = open("/dev/nmdm1B", O_RDWR);
	assert(fo >= 0);
	AZ(tcgetattr(fo, &t));
	cfmakeraw(&t);
	cfsetspeed(&t, B9600);
	t.c_cflag |= CLOCAL | CSTOPB;
	// t.c_cflag |= CDSR_OFLOW;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	AZ(tcsetattr(fo, TCSAFLUSH, &t));

	assert(fi != NULL);
	dof = ReadDomusObj(xfgetc, fi, "-", 0);
	doo = TAILQ_FIRST(&dof->objs);
	assert(doo != NULL);
	for (u = 0; u < 64; u++)
		PC(0);

	PC(0x01);

	amax = 0;
	printf("Loading");
	TAILQ_FOREACH(dor, &doo->recs, list) {
		if (WVAL(dor->w[0]) != 2)
			continue;
		a = WVAL(dor->w[6]);
		printf(" %04x", a);
		for (u = 7; u < dor->nw; u++, a++) {
			assert(WRELOC(dor->w[u]) == 1);
			core[a] = WVAL(dor->w[u]);
			PW(core[a]);
			if (a > amax)
				amax = a;
		}
	}
	printf("\n");
	printf("Last address = 0x%x\n", amax);

	// Flush serial port.
	for (u = 0; u < 3; u++)
		while (read(fo, &c, 1) > 0)
			continue;

	for (u = 0; u < 64; u++) {
		PC(0);
		i = read(fo, &c, 1);
		printf(".");
		if (i == 1 && c == 0) {
			i = read(fo, &c, 1);
			printf("0");
			if (i == 1 && c == 0) {
				printf(" Sync!\n");
				break;
			}
		}
	}

	rx_count = 0;
	tx_count = 0;
	t.c_cc[VTIME] = 200;
	AZ(tcsetattr(fo, TCSAFLUSH, &t));

	Sync();
	i = ChkSum(0x0000, 0x0020);
	memset(card, 0x7f, sizeof card);
	Upload(0x1000, card, 80);
	Download(0x1000, 80, card);
	Fill(0x1000, 0x1080, 0x1234);
	Fill(0x2000, 0x2080, 0x1234);
	Compare(0x1000, 0x2000, 0x0080);
	Fill(0x207f, 0x2080, 0x1235);
	Compare(0x1000, 0x2000, 0x0080);
	Fill(0x207f, 0x2080, 0x1233);
	Compare(0x1000, 0x2000, 0x0080);
	Fill(0x207f, 0x2080, 0x1234);
	Fill(0x2000, 0x2001, 0x1233);
	Compare(0x1000, 0x2000, 0x0080);
	Fill(0x2000, 0x2001, 0x1235);
	Compare(0x1000, 0x2000, 0x0080);
	FreeMem();
	DoCmd(100, 0, 0, 0, 0, NULL, 0, card, 1);

	if (1) {
		DKP_smartdownload("/tmp/_.dkp");
		exit (0);
	}

	while (0) {
		printf("Press enter to read card:");
		fgets(buf, sizeof buf, stdin);
		SendCmd(8, 0x1000, 041, 0, 0);
		i = GW();
		printf("Start = %04x\n", i);
		while (1) {
			SendCmd(9, 0x1000, 0, 0, 0);
			i = GW();
			printf("Busy = %04x, OK ? ", i);
			fgets(buf, sizeof buf, stdin);
			if (buf[0] != '\n')
				break;
		}
		SendCmd(10, 0x1000, 0x1000 + 80, 0, 0);
		i = GW();

		SendCmd(3, 0x1000, 0x1000 + 80, 0, 0);
		for (u = 0; u < 80; u++)
			card[u] = GW();

		for (u = 0; u < 80; u++)
			printf("%x", (card[u] >> 8) & 0xf);
		printf("\n");
		for (u = 0; u < 80; u++)
			printf("%x", (card[u] >> 4) & 0xf);
		printf("\n");
		for (u = 0; u < 80; u++)
			printf("%x", (card[u] >> 0) & 0xf);
		printf("\n");
		for (j = 0x8000; j; j >>= 1) {
			for (i = 0; i < 80; i++) {
				if (card[i] & j)
					printf("#");
				else
					printf("|");
			}
			printf("  ");
			for (i = 0; i < 80; i += 2) {
				if ((card[i]|card[i+1]) & j)
					printf("#");
				else
					printf("|");
			}
			printf("\n");
		}
		printf("Download: %04x\n", GW());
	}
	return (0);
}
