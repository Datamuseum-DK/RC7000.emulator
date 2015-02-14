#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "domus.h"
#include "domusobj.h"
#include "rc3600.h"
#include "rc3600_emul.h"

#define C1B0	(0x8000 >> 0)
#define C1B1	(0x8000 >> 1)
#define C1B2	(0x8000 >> 2)
#define C1B3	(0x8000 >> 3)
#define C1B4	(0x8000 >> 4)
#define C1B5	(0x8000 >> 5)
#define C1B6	(0x8000 >> 6)
#define C1B7	(0x8000 >> 7)
#define C1B8	(0x8000 >> 8)
#define C1B9	(0x8000 >> 9)
#define C1B10	(0x8000 >> 10)
#define C1B11	(0x8000 >> 11)
#define C1B12	(0x8000 >> 12)
#define C1B13	(0x8000 >> 13)
#define C1B14	(0x8000 >> 14)
#define C1B15	(0x8000 >> 15)

#define ADR0	0x1ddd

/**********************************************************************/

static uint16_t	core[32768];

uint16_t
cr(uint16_t a)
{
	assert(a < 0x8000);
	if (a < ADR0 && core[a] == 0) {
		printf("Blank core @ %04x\n", a);
		exit(0);
	}
	// printf(" R[%04x] = %04x ", a, core[a]);
		
	return (core[a]);
}

void
cw(uint16_t a, uint16_t d)
{
	// printf(" W[%04x] = %04x ", a, d);
	assert(a < 0x8000);
	core[a] = d;
}

/**********************************************************************/

static char *
getstr(uint16_t a, uint16_t len)
{
	char s[len + 1];
	int i;

	for (i = 0; i < len; i++) {
		if (i & 1)
			s[i] = core[(a + i) >> 1] & 0xff;
		else
			s[i] = core[(a + i) >> 1] >> 8;
	}
	s[i] = '\0';
	return (strdup(s));
}

/**********************************************************************/

static int
xfgetc(void *priv)
{
	return fgetc(priv);
}

static uint16_t
load_file(const char *fn, uint16_t adr)
{
	struct domus_obj_file *dof;
	struct domus_obj_obj *doo;
	struct domus_obj_rec *dor;
	unsigned u, a, h = 0;
	FILE *fi;
	char buf[20];

	sprintf(buf, "__.%s", fn);
	fi = fopen(buf, "r");
	dof = ReadDomusObj(xfgetc, fi, "-", 0);
	assert(dof != NULL);
	doo = TAILQ_FIRST(&dof->objs);
	assert(doo != NULL);
	TAILQ_FOREACH(dor, &doo->recs, list) {
		if (WVAL(dor->w[0])!= 2)
			continue;
		assert(WRELOC(dor->w[6]) == 2);
		a = WVAL(dor->w[6] + adr);
		for (u = 7; u < dor->nw; u++) {
			// printf("%04x %c ", WVAL(dor->w[u]), fmtreloc[WRELOC(dor->w[u])]);
			switch(WRELOC(dor->w[u])) {
			case 1:
				core[a++] = WVAL(dor->w[u]);
				break;
			case 2:
				core[a++] = WVAL(dor->w[u]) + adr;
				break;
			case 3:
				core[a++] = WVAL(dor->w[u]) + (adr << 1);
				break;
			default:
				exit(2);
			}
			if (a > h)
				h = a;
		}
		//printf("\n");
	}

	printf("Highest Load %04x\n", h);
	return (doo->start + adr);
}



/**********************************************************************/

static TAILQ_HEAD(, process) process_list = TAILQ_HEAD_INITIALIZER(process_list);

struct process *
new_proc(const char *name, sendmsg_f *sendm)
{
	struct process *p;

	assert(name != NULL);
	assert(sendm != NULL);

	p = calloc(sizeof *p, 1);
	assert(p != NULL);
	p->sendm = sendm;
	strncpy(p->name, name, sizeof p->name);
	TAILQ_INSERT_TAIL(&process_list, p, list);
	return (p);
}

/**********************************************************************/
static void
proc_TTY_sendm(struct process *cur, uint16_t msg)
{
	char *txt, *p;
	(void)cur;

	assert(core[msg + 0] == 3);
	txt = getstr(core[msg + 2], core[msg + 1]);
	for (p = txt; *p != '\0'; p++) {
		if (*p >= ' ' && *p <= '~')
			putchar(*p);
		else
			printf("\\x%02x", *p);
	}
	core[msg + 0] = 0;
}

/**********************************************************************/

static void
proc_S_sendm(struct process *cur, uint16_t msg)
{
	char *cmd;

	(void)cur;
	if (core[msg] == 3) {
		cmd = getstr(core[msg + 2], core[msg + 1]);
		printf(" <%s>\n", cmd);
		assert(!strcmp(cmd, "GET DOMWK"));

		core[0xf1] = 0x18c9;

		core[0x18c9] = 1;
		core[0x18ca] = 1;
		core[0x18cb] = 0x1b58;

		core[0x1afa] = 0x5300;
		core[0x1afb] = 0x0000;
		core[0x1afc] = 0x0000;

		core[0x1b58] = 0x166a;
		core[0x1b59] = 1;
		core[0x1b5a] = 0x1db7;

		core[0x1db7] = 0x1af6;
		core[0x1db8] = 1;
		core[0x1db9] = 0x1dd6;

		core[0x1dd6] = 0x1af6;
		core[0x1dd7] = 1;
		core[0x1dd8] = 0x44c3;

		core[0x44c3] = 0x4450;
		core[0x44c4] = 0x44ca;
		core[0x44c5] = 0;
		core[0x44c6] = 0x3856;

		uint16_t load_addr = 0x449c;
		core[msg + 0] = 0;
		core[msg + 2] = load_addr + 7;
		return;
	}
	if (core[msg] == C1B8) {
		cmd = getstr(core[msg + 2], core[msg + 1]);
		printf(" <%s>\n", cmd);
		assert(0);
	}
}

/**********************************************************************/

struct area {
	const char		*fname;
	int			fd;
};

static void
proc_area_sendm(struct process *cur, uint16_t msg)
{
	struct area *a;
	uint8_t buf[512];
	int i, j;

	a = cur->priv;
	switch(core[msg + 0] & 0x3) {
	case 0x0:
	case 0x2:
		printf(" Control");
		/* Control */
		core[msg + 0] = 0;
		core[msg + 1] = 0;
		core[msg + 2] = 0;
		core[msg + 3] = 0;
		break;
	case 0x1:
		printf(" Input");
		assert(core[msg + 3] < 0x200);
		assert((core[msg + 0] & C1B8) == 0);
		assert(core[msg + 1] == 512);
		assert((core[msg + 2] & 1) == 0);
		i = pread(a->fd, buf, 512, core[msg + 3] * 512);
		if (i == 0) {
			memset(buf, 0, sizeof buf);
			i = 512;
		}
		assert(i == 512);
		j = core[msg + 2] >> 1;
		for (i = 0; i < 256; i++, j++) {
			core[j] = 0;
			core[j] |= buf[i + i + 0] << 8;
			core[j] |= buf[i + i + 1];
		}
		core[msg + 0] = 0;
		break;
	case 0x3:
		printf(" Output");
		assert(core[msg + 3] < 0x200);
		assert((core[msg + 0] & C1B8) == 0);
		assert(core[msg + 1] == 512);
		assert((core[msg + 2] & 1) == 0);
		j = core[msg + 2] >> 1;
		for (i = 0; i < 256; i++, j++) {
			buf[i + i + 0] = core[j] >> 8;
			buf[i + i + 1] = core[j] & 0xff;
		}
		i = pwrite(a->fd, buf, 512, core[msg + 3] * 512);
		assert(i == 512);
		core[msg + 0] = 0;
		break;
	}
}


/**********************************************************************/

static void
proc_CAT_sendm(struct process *cur, uint16_t msg)
{
	char fn[18], *fname;
	int i, j, k, fd;
	struct stat st;
	struct process *p;
	struct area *a;

	(void)cur;
	fname = getstr(core[msg + 1] << 1, 5);
	sprintf(fn, "__.%s", fname);

	switch(core[msg]) {
	case C1B1:
		printf(" CREATE <%s>\n", fname);
		fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		assert(fd >= 0);
		assert(ftruncate(fd, core[msg + 2] * 512) == 0);
		assert(close(fd) == 0);
		core[msg + 0] = 0;
		break;
	case C1B2:
		printf(" LOOKUP <%s>\n", fname);

		j = stat(fn, &st);
		if (j < 0) {
			core[msg + 0] = C1B3 + C1B1;
			return;
		}

		i = core[msg + 2];

		core[i+0] = 0x0000;
		core[i+1] = 0x0000;
		core[i+2] = 0x0000;

		for (k = 0; k < 6; k++) {
			if (fname[k] == '\0')
				break;
			if (k & 1)
				core[k >> 1] |= fname[k];
			else
				core[k >> 1] |= fname[k] << 8;
		}
		i += 3;

		core[i++] = 0x0000;			// OPTIONAL
		core[i++] = 0x0000;
		core[i++] = 0x0000;

		core[i++] = C1B15;			// Attr: EXTENDABLE
		core[i++] = (st.st_size + 511) >> 9;	// FILE LENGTH
		core[i++] = 0x3456;			// Index Block sector
		core[i++] = (st.st_size + 511) >> 9;	// Reserved

		core[i++] = 0x0000;			// OPTIONAL
		core[i++] = 0x0000;
		core[i++] = 0x0000;
		core[i++] = 0x0000;
		core[i++] = 0x0000;
		core[i++] = 0x0000;

		core[msg + 0] = 0;
		break;
	case C1B4:
		printf(" DELETE <%s>\n", fname);
		unlink(fn);
		core[msg + 0] = 0;
		break;
	case C1B5:
		printf(" CREATE AREA <%s>\n", fname);
		TAILQ_FOREACH(p, &process_list, list) {
			if (!strcmp(p->name, fname)) {
				core[msg + 0] = 0;
				return;
			}
		}
		p = new_proc(fname, proc_area_sendm);
		a = calloc(sizeof *a, 1);
		a->fname = strdup(fname);
		a->fd = open(fn, O_RDWR | O_CREAT, 0644);
		assert(a->fd >= 0);
		p->priv = a;
		core[msg + 0] = 0;
		break;
	case C1B6:
		printf(" REMOVE AREA <%s>\n", fname);
		core[msg + 0] = 0;
		break;
	default:
		exit(0);
	}
}

/**********************************************************************/

static void
domus_sendm(void)
{
	struct process *p;
	char *name;
	int i;

	name = getstr(acc[2] << 1, 5);
	printf("\tTo: '%s'", name);
	printf(" [%04x", core[acc[1] + 0]);
	printf(" %04x", core[acc[1] + 1]);
	printf(" %04x", core[acc[1] + 2]);
	printf(" %04x]", core[acc[1] + 3]);

	acc[2] = 0x500;
	for (i = 0; i < 4; i++)
		core[0x500 + 6 + i] = core[acc[1] + i];

	TAILQ_FOREACH(p, &process_list, list) {
		if (!strncmp(p->name, name, sizeof p->name)) {
			p->sendm(p, 0x500 + 6);
			acc[3] = core[32];
			pc++;
			return;
		}
	}
	printf("\n");
	fprintf(stderr, "NO SUCH PROCESS <%s>\n", name);
	exit(1);
}

/**********************************************************************/

static void
domus_waita(void)
{

	printf("\t[%04x", core[0x500 + 6]);
	printf(" %04x", core[0x500 + 7]);
	printf(" %04x", core[0x500 + 8]);
	printf(" %04x]\n", core[0x500 + 9]);
	acc[0] = core[0x500 + 6];
	acc[1] = core[0x500 + 7];
	pc++;
}

static void
domus_open(void)
{
	char *z = getstr(acc[2] << 1, 5);
	char fn[10];
	int fd;

	sprintf(fn, "__.%s", z);

	printf("fn = %s", fn);

	if (acc[0] == 3)
		fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	else if (acc[0] == 1)
		fd = open(fn, O_RDONLY, 0);
	else {
		assert(acc[0] == 1);
		fd = -1;
	}
	assert(fd > 0);
	core[acc[2] + 21] = fd;
	printf("Open %s -> %d\n", fn, fd);
	pc++;
}

static void
domus_close(void)
{
	char *z = getstr(acc[2] << 1, 5);
	int fd;

	fd = core[acc[2] + 21];
	core[acc[2] + 21] = 0;
	printf(" %s -> %d\n", z, fd);
	assert(close(fd) == 0);
	pc++;
}

static void
domus_outchar(void)
{
	int fd;
	char c;

	fd = core[acc[2] + 21];
	assert(fd > 0);
	c = acc[1];
	write(fd, &c, 1);
	pc++;
}

static void
domus_inchar(void)
{
	char *z = getstr(acc[2] << 1, 5);
	int fd, i;
	char c;

	fd = core[acc[2] + 21];
	printf(" z=%s fd=%d", z, fd);
	assert(fd > 0);
	i = read(fd, &c, 1);
	if (i != 1)
		c = 0x19;
	acc[1] = c;
	if (c >= ' ' && c <= '~')
		printf(" '%c'\n", c);
	else
		printf(" 0x%02x\n", c);
	pc++;
}

static void
domus_getbyte(void)
{
	int i, j;

	i = acc[1];
	j = core[i >> 1];
	if (i & 1)
		acc[0] = j & 0xff;
	else
		acc[0] = j >> 8;
	acc[2] = core[32];
	pc++;
}

/**********************************************************************/

static void
constants()
{
	int i;
	for (i = 0; i < 16; i++)
		core[65 + i] = 0x8000 >> i;
	core[81 +  0] = 3;
	core[81 +  1] = 5;
	core[81 +  2] = 6;
	core[81 +  3] = 7;
	core[81 +  4] = 9;
	core[81 +  5] = 10;
	core[81 +  6] = 12;
	core[81 +  7] = 13;
	core[81 +  8] = 15;
	core[81 +  9] = 24;
	core[81 + 10] = 25;
	core[81 + 11] = 40;
	core[81 + 12] = 48;
	core[81 + 13] = 56;
	core[81 + 14] = 60;
	core[81 + 15] = 63;
	core[81 + 16] = 120;
	core[81 + 17] = 127;
	core[81 + 18] = 255;
	core[81 + 19] = 65536 - 3;
	core[81 + 20] = 65536 - 4;
	core[81 + 21] = 65536 - 16;
	core[81 + 22] = 65536 - 256;
}

/**********************************************************************
 * RCSL_43_GL_8374
 * NB: only 'name' supported
 */

static void
proc_S_build_arg(int adr, int argc, const char * const *argv)
{
	const char *p;
	int i, sep = 0;
	(void)adr;
	(void)argc;
	(void)argv;
	while (--argc) {
		p = *++argv;
		do {
			i = strcspn(p, ",./:=");
			assert(i > 0);
			printf("%d <%.*s> %02x %d\n", argc, i, p, p[i], i);
			core[adr++] = 0x000a;
			core[adr++] = sep << 8;
			core[adr + 0] = 0x0000;
			core[adr + 1] = 0x0000;
			core[adr + 2] = 0x0000;
			if (i > 0) core[adr + 0] |= p[0] << 8;
			if (i > 1) core[adr + 0] |= p[1];
			if (i > 2) core[adr + 1] |= p[2] << 8;
			if (i > 3) core[adr + 1] |= p[3];
			if (i > 4) core[adr + 2] |= p[4] << 8;
			sep = p[i];
			adr += 3;
			p += i;
			if (*p)
				p++;
		} while (*p);
	}
	core[adr++] = 0x0000;
	core[adr++] = 0x0004;
}

static void
zone(void)
{
	char *z = getstr(acc[2] << 1, 5);

	printf(" ZONE=0x%04x '%s'", acc[2], z);
	free(z);
}

int
main(int argc, const char * const *argv)
{
	intmax_t i, j;
	uint16_t cur;
	char *p;

	assert(argc > 1);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	(void)new_proc("TTY", proc_TTY_sendm);
	(void)new_proc("S", proc_S_sendm);
	(void)new_proc("CAT", proc_CAT_sendm);

	constants();

	cur = load_file(argv[1], ADR0);

	printf("CUR = %04x\n", cur);

	pc = core[cur + 19] >> 1;

	acc[1] = 0x4497;
	proc_S_build_arg(acc[1], argc, argv);

	acc[2] = 0;
	core[32] = cur;

	if (0) {
		for (i = 0; i < 32768;) {
			printf("%04jx: ", i);
			for (j = 0; j < 16; j++)
				printf(" %04x", core[i + j]);
			printf("\n");
			i += j;
		}
	}

	for (i = 0; i < 100000000LL; i++) {
		if (0) {
			p = Domus3Disass(core[pc], NULL, NULL);
			printf("%10jd %04x %04x [%04x %04x %04x %04x] %s", i,
			    pc, core[pc], acc[0], acc[1], acc[2], acc[3], p);
		}
		switch(core[pc]) {
		case 0x0c04:	// SENDMESSAGE
			p = Domus3Disass(core[pc], NULL, NULL);
			printf("%10jd %04x %04x [%04x %04x %04x %04x] %s", i,
			    pc, core[pc], acc[0], acc[1], acc[2], acc[3], p);
			domus_sendm();
			printf("\n");
			break;
		case 0x0c05:	// WAITANSWER
			p = Domus3Disass(core[pc], NULL, NULL);
			printf("%10jd %04x %04x [%04x %04x %04x %04x] %s", i,
			    pc, core[pc], acc[0], acc[1], acc[2], acc[3], p);
			domus_waita();
			printf("\n");
			break;
		case 0x0ce6: zone(); pc++; break; // CREATEENTRY
		case 0x0c91: domus_open(); break;
		case 0x0c90: domus_close(); break;
		case 0x0c8f: zone(); pc++; break; // SETPOSITION
		case 0x0c8a: domus_outchar(); break;
		case 0x0c87: domus_inchar(); break;
		case 0x0c7c: domus_getbyte(); break;
		default:
			rc3600_exec();
			break;
		}
		if (0)
			printf("\n");
	}

	return(0);
}

