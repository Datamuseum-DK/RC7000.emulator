#include <assert.h>
#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <sys/endian.h>
#include <sys/types.h>

#include <rc3600.h>
#include <domusobj.h>

uint32_t CUR = WDEFCUR;

const char fmtreloc[8] = "0 \'\"456*";

uint32_t
Wtonorm(uint32_t a)
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

uint32_t
Woffset(uint32_t a, int i)
{
	uint32_t w;

	w = a + i;
	w &= ~WOFLOW;
	return (w);
}

const char *
Wfmt(uint32_t w, char *buf)
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
Wsetabs(uint32_t *w, u_int val)
{

	*w = (val & WVMASK) | WVALID | (RABS << WRSHIFT);
}


static int
GetWord(struct domus_obj_file *fp, uint16_t *w)
{
	int i;
	u_char b[2];

	do {
		i = fp->func(fp->priv);
		if (i < 0)
			return (EOF);
		b[0] = i;
	} while (fp->in_leader && i == 0);
	fp->in_leader = 0;

	i = fp->func(fp->priv);
	if (i < 0)
		return (EOF);
	b[1] = i;
	*w = le16dec(b);
	return (0);
}

/*
 * Read the file and create "struct rec" records on a list
 */

struct domus_obj_file *
ReadDomusObj(getc_f *func, void *priv, const char *fn, int verbose)
{
	uint16_t type, len, sum, tmp;
	struct domus_obj_obj *op;
	struct domus_obj_rec *rp;
	struct domus_obj_file *fp;
	u_int i;
	char buf[20];

	assert(func != NULL);
	
	fp = calloc(sizeof *fp, 1);
	assert(fp != NULL);

	fp->fn = strdup(fn);
	TAILQ_INIT(&fp->objs);

	fp->func = func;
	fp->priv = priv;
	fp->in_leader = 1;

	op = NULL;
	if (verbose)
		printf(
		    " type len_ rel1|reloc rel2|reloc rel3|reloc sum_ addr\n");
	for (;;) {
		if (GetWord(fp, &type))
			break;
		if (type == 0) {
			if (GetWord(fp, &type))
				break;
		}
		if (type == 0 || type > 0x0009)
			break;
		if (GetWord(fp, &len))
			errx(1, "Premature EOF on file %s in 0x%04x record\n",
			    fn, type);
		if (op == NULL) {
			op = calloc(sizeof *op, 1);
			TAILQ_INIT(&op->recs);
			TAILQ_INSERT_TAIL(&fp->objs, op, list);
		}
		rp = calloc(sizeof *rp, 1);
		rp->w[0] = type | WVALID;
		rp->w[1] = len | WVALID;
		for (i = 2; i < 6 + 65536U - len; i++) {
			if (GetWord(fp, &tmp))
				errx(1, "Premature EOF on file %s"
				   " in 0x%04x record\n", fn, type);
			rp->w[i] = tmp | WVALID;
		}
		rp->nw = i;
		sum = 0;
		for (i = 0; i < rp->nw; i++) {
			if (verbose)
				printf(" %04x", WVAL(rp->w[i]));
			if (i >= 2 && i <= 4 && verbose)
				printf("|%05o", WVAL(rp->w[i]) >> 1);
			sum += WVAL(rp->w[i]);
		}
		if (sum != 0)
			errx(1, " Checksum error: %04x", sum);
		if (verbose)
			printf("\n");
		TAILQ_INSERT_TAIL(&op->recs, rp, list);
		switch (WVAL(rp->w[0])) {
		case 2:
			sprintf(buf, "%05o%05o%05o",
			    WVAL(rp->w[2])/2,
			    WVAL(rp->w[3])/2,
			    WVAL(rp->w[4])/2);
			for (i = 6; i < rp->nw; i++)
				rp->w[i] |= (buf[i - 6] - '0') << WRSHIFT;
			break;
		case 6:
			sprintf(buf, "%05o%05o%05o",
			    WVAL(rp->w[2])/2,
			    WVAL(rp->w[3])/2,
			    WVAL(rp->w[4])/2);
			for (i = 6; i < rp->nw; i++)
				rp->w[i] |= (buf[i - 6] - '0') << WRSHIFT;
			op->start = rp->w[6];
			if (verbose)
				printf("END of OBJECT title %s start %s\n",
				    op->title, Wfmt(op->start, buf));
			op = NULL;
			fp->in_leader = 1;
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
	return(fp);
}
