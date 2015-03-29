
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/queue.h>

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

struct sd {
	TAILQ_ENTRY(sd)	list;
	uint16_t		addr;
	uint16_t		sum;
	uint16_t		len;
	uint16_t		c,h,s;
	uint16_t		valid;
	uint16_t		w[256];
};

void
DKP_smartdownload(const char *fn)
{
	FILE *ft;
	TAILQ_HEAD(,sd)		head;
	uint16_t a0, a, b, c, lc, h, s;
	struct sd *ss, *ss2;
	uint16_t ret[128];
	uint16_t chew[24];
	int i, j, hits = 0, miss = 0, rpts = 0, zeros = 0;

	ft = fopen(fn, "w");
	if (ft == NULL) {
		perror(fn);
		exit(2);
	}

	TAILQ_INIT(&head);
	a0 = a = FreeMem();
	a += 0xc00;
	b = 0;
	while (a + 0x100 <= 0x8000) {
		ss = calloc(sizeof *ss, 1);
		assert(ss != NULL);
		ss->addr = a;
		TAILQ_INSERT_TAIL(&head, ss, list);
		a += 0x100;
		b++;
	}
	printf("%u buffers\n", b);

	printf("Recal: %04x\n", DKP_recal());
	lc = 0xffff;

	for (c = 0; c < 203; c++) {
		if (c != lc) {
			i = DKP_seek(c);
			printf("Seek: cyl=%d %04x\n", c, i);
			assert(i == 0x4040);
			lc = c;
		}
		for (h = 0; h < 2; h++) {
			printf("===== cyl %d head %d =====\n", c, h);

			DKP_rw(c, a0, 0x0004 | (h << 8), ret);
			printf("DKP: DIA=%04x", ret[0]);
			assert(ret[0] == 0xc040);
			printf(" DIB=%04x", ret[1]);
			assert(ret[1] == a0 + 0xc00);
			printf(" DIC=%04x\n", ret[2]);
			assert(ret[2] == ((12 << 4) | (h << 8)));
			Chew(a0, 0x100, 12, chew);

			for (s = 0; s < 12; s++) {
				if (chew[s + s + 1] == 0x100) {
					for (j = 0; j < 512; j++)
						fputc(0, ft);
					zeros++;
					printf("zero %d/%d/%d\n", c, h, s);
					continue;
				}
				a = a0 + s * 0x100;
				ss = TAILQ_FIRST(&head);
				ss->sum = chew[s + s];
				ss->c = c;
				ss->h = h;
				ss->s = s;
				ss->valid = 0;
				i = 1;
				TAILQ_FOREACH(ss2, &head, list) {
					if (!ss2->valid)
						continue;
					if (ss2->sum != ss->sum)
						continue;
					i = Compare(a, ss2->addr, ss2->len);
					if (i == 0) {
						printf("hit %d/%d/%d", c, h, s);
						printf("= %d/%d/%d %04x %d\n",
						    ss2->c, ss2->h, ss2->s,
						    ss->sum, chew[s+s+1]);
						ss = ss2;
						hits++;
						break;
					} else {
						miss++;
						printf("miss %d/%d/%d", c, h, s);
						printf("= %d/%d/%d %04x %d\n",
						    ss2->c, ss2->h, ss2->s,
						    ss->sum, i);
					}
				}
				if (i) {
					rpts += chew[s + s + 1];
					j = 0x100 - chew[s + s + 1];
					ss->len = j;
					DownloadAndMove(a, j, ss->addr, ss->w);
					for (;j < 0x100; j++)
						ss->w[j] = ss->w[j - 1];
					ss->valid = 1;
				}
				for (j = 0; j < 256; j++) {
					i = ss->w[j];
					fputc(i >> 8, ft);
					fputc(i & 0xff, ft);
				}
				TAILQ_REMOVE(&head, ss, list);
				TAILQ_INSERT_TAIL(&head, ss, list);
			}
		}
	}
	printf("RX count %lu bytes\n", rx_count);
	printf("TX count %lu bytes\n", tx_count);
	printf("CMD count %lu\n", cmd_count);
	printf("Zero %d sectors\n", zeros);
	printf("Cache hit %d sectors\n", hits);
	printf("Cache miss %d sectors\n", miss);
	printf("Repeat %d words\n", rpts);
}

/**********************************************************************
 */

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
