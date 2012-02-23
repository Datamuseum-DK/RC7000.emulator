#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sys/endian.h>
#include <sys/types.h>

#include <rc3600.h>
#include <domusobj.h>

W CUR = WDEFCUR;

const char fmtreloc[8] = "0 \'\"456*";

W
Wtonorm(W a)
{
	if (!WISVALID(a))
		return (0);

	switch(WRELOC(a)) {
	case RNORM:
	case RHIGH:
		return (a);
	case RBYTE:
		return ((WVAL(a) >> 1) | WVALID) | (RNORM << WRSHIFT);
	default:
		return (0);
	}
}

W
Woffset(W a, int i)
{
	W w;

	w = a + i;
	w &= ~WOFLOW;
	return (w);
}

const char *
Wfmt(W w, char *buf)
{
	static char mybuf[6];

	if (buf == NULL)
		buf = mybuf;

	if (w == CUR)
		return ("CUR  ");
	if (WISVALID(w)) {
		sprintf(buf, "%04x%c", WVAL(w), fmtreloc[WRELOC(w)]);
		return (buf);
	}
	if (w & WMSG)
		return ("mess ");
	return ("-----");
}

void
Wsetabs(W *w, u_int val)
{

	*w = (val & WVMASK) | WVALID | (RABS << WRSHIFT);
}


static int
GetWord(FILE *f, uint16_t *w)
{
	u_char b[2];

	if (fread(b, sizeof b, 1, f) != 1)
		return (EOF);
	*w = le16dec(b);
	return (0);
}

static void
SkipLeader(FILE *f)
{
	int i;

	for(;;) {
		i = getc(f);
		if (i == EOF)
			return;
		else if (i != 0) {
			ungetc(i, f);
			return;
		}
	}
}

/*
 * Read the file and create "struct rec" records on a list
 */

struct domus_obj_file *
ReadDomusObj(FILE *f, const char *fn)
{
	uint16_t type, len, sum, tmp;
	struct domus_obj_obj *op;
	struct domus_obj_rec *rp;
	struct domus_obj_file *fp;
	u_int i;
	char buf[20];

	if (f == NULL)
		f = fopen(fn, "r");
	if (f == NULL)
		err(1, "File not open %s", fn);
	fp = calloc(sizeof *fp, 1);
	fp->fn = strdup(fn);
	TAILQ_INIT(&fp->objs);
	op = NULL;
	printf(" type len_ rel1|reloc rel2|reloc rel3|reloc sum_ addr\n");
	SkipLeader(f);
	for (;;) {
		if (GetWord(f, &type))
			break;
		if (type == 0) {
			if (GetWord(f, &type))
				break;
		}
		if (type == 0 || type > 0x0009)
			break;
		if (GetWord(f, &len))
			errx(1, "Premature EOF on file %s in 0x%04x record\n", fn, type);
		if (op == NULL) {
			op = calloc(sizeof *op, 1);
			TAILQ_INIT(&op->recs);
			TAILQ_INSERT_TAIL(&fp->objs, op, list);
		}
		rp = calloc(sizeof *rp, 1);
		rp->w[0] = type | WVALID;
		rp->w[1] = len | WVALID;
		for (i = 2; i < 6 + 65536U - len; i++) {
			if (GetWord(f, &tmp))
				errx(1, "Premature EOF on file %s in 0x%04x record\n", fn, type);
			rp->w[i] = tmp | WVALID;
		}
		rp->nw = i;
		sum = 0;
		for (i = 0; i < rp->nw; i++) {
			printf(" %04x", WVAL(rp->w[i]));
			if (i >= 2 && i <= 4)
				printf("|%05o", WVAL(rp->w[i]) >> 1);
			sum += WVAL(rp->w[i]);
		}
		if (sum != 0)
			errx(1, " Checksum error: %04x", sum);
		printf("\n");
		TAILQ_INSERT_TAIL(&op->recs, rp, list);
		switch (WVAL(rp->w[0])) {
		case 2:
			sprintf(buf, "%05o%05o%05o", 
			    WVAL(rp->w[2])/2, WVAL(rp->w[3])/2, WVAL(rp->w[4])/2);
			for (i = 6; i < rp->nw; i++)
				rp->w[i] |= (buf[i - 6] - '0') << WRSHIFT;
			break;
		case 6:
			sprintf(buf, "%05o%05o%05o", 
			    WVAL(rp->w[2])/2, WVAL(rp->w[3])/2, WVAL(rp->w[4])/2);
			for (i = 6; i < rp->nw; i++)
				rp->w[i] |= (buf[i - 6] - '0') << WRSHIFT;
			op->start = rp->w[6];
			printf("END of OBJECT title %s start %s\n",
			    op->title, Wfmt(op->start, buf));
			op = NULL;
			SkipLeader(f);
			break;
		case 7:
			Radix40(WVAL(rp->w[6]), WVAL(rp->w[7]), op->title);
			break;
		default:
			break;
		}
		if (type == 6) {
		}
	} 
	fclose (f);
	return(fp);
}
