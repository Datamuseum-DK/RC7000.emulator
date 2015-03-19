
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

/**********************************************************************
 * Slave calls
 */

static uint16_t
DKP_recal(void)
{
	uint16_t retval;

	printf("RECAL\t");
	DoCmd(20, 0, 0, 0, 0, NULL, 0, &retval, 1);
	return (retval);
}

static void
DKP_rw(uint16_t doa, uint16_t dob, uint16_t doc, uint16_t *result)
{

	printf("RW\t");
	DoCmd(21, doa, dob, doc, 0, NULL, 0, result, 3);
}

static uint16_t
DKP_seek(uint16_t cyl)
{
	uint16_t retval;

	printf("SEEK\t");
	DoCmd(22, cyl, 0, 0, 0, NULL, 0, &retval, 1);
	return (retval);
}

/**********************************************************************
 * Canned functions
 */

void
DKP_download(const char *fn)
{
	FILE *ft;
	int i, u, cyl,lcyl,hd;
	uint16_t ret[12 * 256];

	ft = fopen(fn, "w");
	if (ft == NULL) {
		perror(fn);
		exit(2);
	}

	printf("Recal: %04x\n", DKP_recal());

	lcyl = 0;
	for (cyl = 0; cyl < 203; cyl++) {
		for (hd = 0; hd < 2; hd++) {
			if (cyl != lcyl) {
				i = DKP_seek(cyl);
				printf("Seek: cyl=%d %04x\n", cyl, i);
				assert(i == 0x4040);
				lcyl = cyl;
			}

			DKP_rw(cyl, 0x1000, 0x0004 | (hd << 8), ret);
			printf("DKP: DIA=%04x", ret[0]);
			assert(ret[0] == 0xc040);
			printf(" DIB=%04x", ret[1]);
			assert(ret[1] == 0x1c00);
			printf(" DIC=%04x\n", ret[2]);
			assert(ret[2] == (0x00c0 | (hd << 8)));

			if (0) {
				Download(0x1000, 0x0c00, ret);
				for (u = 0; u < 12*256; u++) {
					i = ret[u];
					fputc(i >> 8, ft);
					fputc(i & 0xff, ft);
				}
			} else {
				ChkSum(0x1000, 0xc00);
			}
			fflush(ft);
		}
	}
}

#if 0

static void
dkp_write_upload_track(FILE *fi)
{
	uint8_t sect[512];
	uint16_t ws[256];
	int i, j, u, sc,s;

	SendCmd(4, 0x1000, 0x1c00, 0, 0);
	GW();
	for (sc = 0; sc < 12; sc++) {
		i = fread(sect, sizeof sect, 1, fi);
		assert(i == 1);
		for (i = 0, j = 0; i < 256; i++, j+=2)
			ws[i] = sect[j] << 8 | sect[j+1];
		for (j = 256; j > 0; j--)
			if (ws[255] != ws[j - 1])
				break;
		if (j > 200)
			j = 256;

		if (j > 0) {
			s = 0x1000 + 0x100 * sc;
			SendCmd(2, s, s + j, 0, 0);
			GW();
			s = 0;
			for (u = 0; u < j; u++) {
				s += ws[u];
				PW(ws[u]);
			}
			i = GW();
			s &= 0xffff;
			printf(" Upload: %3d %04x %04x", j, i, s);
			assert(s == i);
		}
		if (j != 256 && ws[255] != 0) {
			s = 0x1000 + 0x100 * sc;
			SendCmd(4, s + j, s + 0x100, ws[255], 0);
			printf(" Fill: %3d %04x", 256 - j, GW());
		}
		printf("\n");
	}
}

static void
dkp_write(const char *fn)
{
	FILE *fi = fopen(fn, "r");
	int i, cyl,lcyl,hd;

	SendCmd(5, 0, 0, 0, 0);
	printf("Recal: %04x\n", GW());

	lcyl = 0;
	for (cyl = 0; cyl < 203; cyl++) {
		for (hd = 0; hd < 2; hd++) {
			if (cyl != lcyl) {
				SendCmd(7, cyl, 0, 0, 0);
				i = GW();
				printf(" Seek: cyl=%d %04x\n", cyl, i);
				assert(i == 0x4040);
				lcyl = cyl;
			}
			dkp_write_upload_track(fi);

			printf("c%03d h%d ", cyl, hd);

			SendCmd(6, 0x100 | cyl, 0x1000, 0x0004 | (hd << 8), 0);
			i = GW();
			printf("DKP: DIA=%04x", i);
			assert(i == 0xc040);
			i = GW();
			printf(" DIB=%04x", i);
			assert(i == 0x1c02);
			i = GW();
			printf(" DIC=%04x\n", i);
			assert(i == (0x00c0 | (hd << 8))); // emulator
			//assert(i == (0x0004 | (hd << 8))); // live
		}
	}

}

#endif
