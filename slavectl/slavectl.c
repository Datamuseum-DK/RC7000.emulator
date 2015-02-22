
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

#define AZ(foo)         do { assert((foo) == 0); } while (0)
#define AN(foo)         do { assert((foo) != 0); } while (0)

static int fo;

static int
xfgetc(void *priv)
{
        return fgetc(priv);
}

static void
PC(uint8_t u)
{
	// printf(">>%02x\n", u);
	assert(1 == write(fo, &u, 1));
}

static void
PW(unsigned u)
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

static uint16_t core[32768];

static void
read_dkp(const char *fn)
{
	FILE *ft = fopen(fn, "w");
	int i, u, cyl,lcyl,hd;

	assert(ft != NULL);

	SendCmd(5, 0, 0, 0, 0);
	printf("Recal: %04x\n", GW());

	lcyl = 0;
	for (cyl = 0; cyl < 203; cyl++) {
		for (hd = 0; hd < 2; hd++) {
			if (cyl != lcyl) {
				SendCmd(7, cyl, 0, 0, 0);
				i = GW();
				printf("\nSeek: cyl=%d %04x\n", cyl, i);
				assert(i == 0x4040);
				lcyl = cyl;
			}

			SendCmd(6, cyl, 0x1000, 0x0004 | (hd << 8), 0);
			i = GW();
			printf("DKP: DIA=%04x", i);
			assert(i == 0xc040);
			i = GW();
			printf(" DIB=%04x", i);
			assert(i == 0x1c00);
			i = GW();
			printf(" DIC=%04x\n", i);
			assert(i == (0x00c0 | (hd << 8)));

			SendCmd(3, 0x1000, 0x1c00, 0, 0);
			for (u = 0; u < 12*256; u++) {
				i = GW();
				fputc(i >> 8, ft);
				fputc(i & 0xff, ft);
			}
			fflush(ft);
			printf("Download: %04x\n", GW());
		}
	}
}

int
main(int argc, char **argv)
{
        struct domus_obj_file *dof;
        struct domus_obj_obj *doo;
        struct domus_obj_rec *dor;
	FILE *fi;
	unsigned u, a;
	struct termios t;
	uint8_t c;
	int i;

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
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	AZ(tcsetattr(fo, TCSAFLUSH, &t));

	assert(fi != NULL);
	dof = ReadDomusObj(xfgetc, fi, "-", 1);
	doo = TAILQ_FIRST(&dof->objs);
	assert(doo != NULL);
	for (u = 0; u < 64; u++) 
		PC(0);

	PC(0x01);
		
	TAILQ_FOREACH(dor, &doo->recs, list) {
		if (WVAL(dor->w[0]) != 2)
			continue;
		a = WVAL(dor->w[0]);
		printf("---> %04x\n", a);
		for (u = 7; u < dor->nw; u++, a++) {
			assert(WRELOC(dor->w[u]) == 1);
			core[a] = WVAL(dor->w[u]);
			PW(core[a]);
		}
	}

	for (u = 0; u < 3; u++)
		while (read(fo, &c, 1) > 0)
			continue;

	for (u = 0; u < 64; u++) {
		PC(0);
		i = read(fo, &c, 1);
		printf("%d %02x %u\n", i, c, u);
		if (i == 1 && c == 0) {
			i = read(fo, &c, 1);
			printf("  %d %02x %u\n", i, c, u);
			if (i == 1 && c == 0) {
				printf("Sync!\n");
				break;
			}
		}
	}
	t.c_cc[VTIME] = 50;
	AZ(tcsetattr(fo, TCSAFLUSH, &t));

	SendCmd(1, 0x00, 0x20, 0, 0);
	printf("Sum %04x\n", GW());

	SendCmd(1, 0x00, 0x20, 0, 0);
	printf("Sum %04x\n", GW());

	SendCmd(1, 0x1000, 0x1100, 0, 0);
	printf("Sum %04x\n", GW());

	SendCmd(2, 0x1000, 0x1100, 0, 0);
	printf("Ack: %04x\n", GW());
	i = 0;
	for (u = 0; u < 256; u++) {
		i += u;
		PW(u);
	}
	printf("Upload: %04x %04x\n", GW(), i);

	SendCmd(1, 0x1000, 0x1100, 0, 0);
	printf("Sum %04x (%04x)\n", GW(), i & 0xffff);
	
	SendCmd(3, 0x1000, 0x1100, 0, 0);
	for (u = 0; u < 256; u++) {
		assert(u == GW());
	}
	printf("Download: %04x\n", GW());

	SendCmd(4, 0x1000, 0x2000, 0x5555, 0);
	printf("Fill: %04x\n", GW());

	SendCmd(3, 0x1000, 0x1100, 0, 0);
	for (u = 0; u < 256; u++) {
		i = GW();
		assert(0x5555 == i);
	}
	printf("Download: %04x\n", GW());

	read_dkp("/tmp/_.ty");
	return (0);
}
