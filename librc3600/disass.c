#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <rc3600.h>

static const char *alu[8]	= {"COM", "NEG", "MOV", "INC", "ADC", "SUB", "ADD", "AND" };
static const char *carry[4]	= {" ", "Z", "O", "C" };
static const char *shift[4]	= {" ", "L", "R", "S" };
static const char *skip[8]	= {"    ", ",SKP", ",SZC", ",SNC", ",SZR", ",SNR", ",SEZ", ",SBN" };
static const char *hash[2]	= {"   ", " # " };
static const char *jjid[4]	= {"JMP", "JSR", "ISZ", "DSZ" };
static const char *at[2]	= {"   ", " @ " };
static const char *ldst[4]	= {NULL, "LDA", "STA", NULL};
static const char *io[8]	= {"NIO", "DIA", "DOA", "DIB", "DOB", "DIC", "DOC", "SKP" };
static const char *test[4]	= {"BN", "BZ", "DN", "DZ" };
static const char *func[4]	= {"  ", "S ", "C ", "P " };

static void
displ(char *buf, u_int u, int *dsp, const char **pz)
{
	int i, d;

	d = (u >> 8) & 3;
	i = u & 0x3ff;

	if (d == 0 && pz != NULL && pz[i] != NULL) {
		strcpy(buf, pz[i]);
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

char *
Rc3600Disass(uint16_t u, struct disass_magic *magic, const char **pz, const char **iodev, char *buf, int *offset)
{
	int i;
	static char mybuf[20];
	struct disass_magic const *sp;

	if (buf == NULL)
		buf = mybuf;

	if (offset != NULL)
		*offset = Rc3600Disass_NO_OFFSET;

	for (sp = magic; sp != NULL && sp->s != NULL; sp++) {
		if (u == sp->n) {
			strcpy(buf, sp->s);
			return (buf);
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
		displ(buf + strlen(buf), u, offset, pz);
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
		displ(buf + strlen(buf), u, offset, pz);
	}
	return (buf);
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
		LagudDisass(buf, u, &d);
		printf("%05d %04x %c%c %04x  %s  ", a, a, ascii[1], ascii[0], u, buf);
		if (d != 0)
			printf("->%04x", a + d);
		printf("\n");
	}
	return(0);
}
#endif
