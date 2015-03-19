
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

uint16_t
ChkSum(uint16_t from, uint16_t len)
{
	uint16_t in;

	printf("CHKSUM\t");
	DoCmd(1, from, len, 0, 0, NULL, 0, &in, 1);
	printf(" => S|%04x\n", in);
	return (in);
}

void
Upload(uint16_t dst, uint16_t *fm, uint16_t len)
{
	uint16_t in[2], s = 0;
	int i;

	printf("UPLOAD\t");
	DoCmd(2, dst, len, 0, 0, fm, len, in, 1);
	for (i = 0; i < len; i++)
		s += fm[i];
	printf(" => S|%04x (%04x)\n", in[0], s);
	assert(s == in[0]);
	ChkSum(dst, len);
}

void
Download(uint16_t src, uint16_t len, uint16_t *to)
{
	uint16_t in[len + 1], s = 0;
	int i;

	printf("DNLOAD\t");
	DoCmd(3, src, src+len, 0, 0, NULL, 0, in, len + 1);
	for (i = 0; i < len; i++) {
		to[i] = in[i];
		s += in[i];
	}
	printf(" => S|%04x (%04x)\n", in[len], s);
	assert(s == in[len]);
}

void
Fill(uint16_t from, uint16_t to, uint16_t val)
{
	uint16_t in[1], s = 0;
	int i;

	assert(to > from);
	printf("FILL\t");
	DoCmd(4, from, to, val, 0, NULL, 0, in, 1);
	for (i = 0; i < (to-from); i++)
		s += val;
	printf(" => S|%04x (%04x)\n", in[0], s);
	assert(s == in[0]);
}

uint16_t
FreeMem(void)
{
	uint16_t retval;

	printf("FREEMEM\t");
	DoCmd(999, 0, 0, 0, 0, NULL, 0, &retval, 1);
	return (retval);
}
