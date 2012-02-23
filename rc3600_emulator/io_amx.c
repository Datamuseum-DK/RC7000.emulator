/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>

#include "rc3600_io.h"

static const double baud[16] = {
	9600, 4800,  2400, 1200, 600, 300, 220, 200,
	 150,  134.5, 110,   75,  50,  40,  40,  40
};

struct amxchan {
	uint8_t			cno;

	int fd;

	uint16_t		ibuf;

	/* DOA stuff */
	uint8_t			rxmode;
	uint8_t			txmode;
	uint8_t			dtr;

	/* incoming modem */
	uint8_t			ri;
	uint8_t			cd;
	uint8_t			dsr;

	/* DOC stuff */
	double			txbaud;
	double			rxbaud;
	unsigned		databits;
	unsigned		stopbits;
	enum {odd, even, none}	parity;

};

struct amx {
	unsigned		dev;

	/* DIA reg */
	uint16_t		dia;

	struct amxchan		ch[8];
};

static void
AMX_Timeout(void *arg, int flag)
{
	struct pollfd pfd[8];
	unsigned u;
	char c;
	int i, irq = 0;

	struct amx *amx = arg;
	(void)flag;
	timeout(100000000, AMX_Timeout, arg, 0);
	for (u = 0; u < 8; u++) {
		pfd[u].fd = amx->ch[u].fd;
		pfd[u].events = POLLIN;
	}
	i = poll(pfd, 8, 0);
	if (i == 0)
		return;
	for (u = 0; u < 8; u++) {
		if (pfd[u].fd < 0)
			continue;
		i = read(pfd[u].fd, &c, 1);
		assert(i == 1);
		amx->ch[u].ibuf = (1<<15) | c;
		irq = 1;
	}
	if (irq) {
		ioprint("AMX%o: IRQ", amx->dev);
		iodevs[amx->dev].busy = 1;
		dev_irq(&iodevs[amx->dev], 0);
	}
}

static void
amx_doc(struct amx *a, uint16_t reg)
{
	struct amxchan *ch = &a->ch[(reg >> 8) & 7];

	ioprint("%04x -> ", reg);
	switch(reg & 0x30) {
	case 0x00:
	case 0x10:
		ch->stopbits = reg & 0x10 ? 2 : 1;
		ch->databits = 5 + (reg & 3);
		switch (reg & 0xc0) {
		case 0x00: ch->parity = odd; break;
		case 0x04: ch->parity = even; break;
		default: ch->parity = none; break;
		}
		ioprint("AMX%o: BITS[%u] = %u/%u/%u", a->dev, ch->cno, ch->databits, ch->stopbits, ch->parity);
		return;
	case 0x20:
		ch->rxbaud = baud[reg & 0xf];
		ioprint("AMX%o: RXBAUD[%u] = %g", a->dev, ch->cno, ch->rxbaud);
		return;
	case 0x30:
		ch->txbaud = baud[reg & 0xf];
		ioprint("AMX%o: TXBAUD[%u] = %g", a->dev, ch->cno, ch->txbaud);
		return;
	}
	ioprint("AMX%o: DOC[%u] 0x%04x", a->dev, ch->cno, reg);
	exit (1);
}

static void
amx_doa(struct amx *a, uint16_t reg)
{
	struct amxchan *ch = &a->ch[(reg >> 8) & 7];

	ioprint("%04x -> ", reg);
	switch (reg & 0x0f) {
	case 0x00:	/* Receive */
		/* XXX: if (reg & 0x10) clear_buffer */
		ch->rxmode = 1;
		ioprint("AMX%o: RXMODE[%u] = %u", a->dev, ch->cno, ch->rxmode);
		return;
	case 0x02:	/* Transmit */
		ch->txmode = 1;
		ioprint("AMX%o: TXMODE[%u] = %u", a->dev, ch->cno, ch->txmode);
		return;
	case 0x03:	/* Stop Transmit */
		ch->txmode = 0;
		ioprint("AMX%o: TXMODE[%u] = %u", a->dev, ch->cno, ch->txmode);
		return;
	case 0x04:	/* Select Input Buffer */
		ioprint("AMX%o: SEL IN BUF[%u] = 0x%04x",
		    a->dev, ch->cno, ch->ibuf);
		a->dia = ch->ibuf; 	/* XXX */
		ch->ibuf = 0x0080;
		return;
	case 0x05:	/* Select Modem Status */
		a->dia = ch->ri ? (1<<15) : 0;
		a->dia |= ch->cd ? 0 : (1<<14);
		a->dia |= ch->dsr ? 0 : (1<<13);
		ioprint("AMX%o: MODEMSTATUS[%u] = %x", a->dev, ch->cno, a->dia);
		return;
	case 0x07:	/* Select Output Buffer Status */
		ioprint("AMX%o: OUT BUF STATUS[%u] = 0xc000", a->dev, ch->cno);
		a->dia = (1 << 15) | (1 << 14);	/* XXX */
		return;
	case 0x08:	/* Set Data Term Ready On */
		ch->dtr = 1;
		ioprint("AMX%o: DTR[%u] = %u", a->dev, ch->cno, ch->dtr);
		return;
	case 0x09:	/* Set Data Term Ready Off */
		ch->dtr = 0;
		ioprint("AMX%o: DTR[%u] = %u", a->dev, ch->cno, ch->dtr);
		return;
	}
	fprintf(stderr, "AMX%o: DOA[%u] 0x%04x", a->dev, ch->cno, reg);
	exit (1);
}

static void
amx_dob(struct amx *a, uint16_t reg)
{
	struct amxchan *ch = &a->ch[(reg >> 8) & 7];
	char c;
		
	c = reg & 0xff;
	if (ch->fd >= 0) {
		assert(1 == write(ch->fd, &c, 1));
	}
	ioprint("AMX%o: DOB[%u] 0x%04x (%c)", a->dev, ch->cno, reg, reg & 0xff);
}



static void
dev_amx(uint16_t ioi, uint16_t *reg, struct iodev *iodev)
{
	struct amx *a = iodev->priv;

	switch (ioi) {
	case NIOC:
		irq_lower(iodev);
		break;
	case DOA:
		amx_doa(a, *reg);
		return;
	case DIA:
		ioprint("AMX%o: DIA 0x%04x)", a->dev, a->dia);
		*reg = a->dia;
		return;
	case DOB:
		amx_dob(a, *reg);
		return;
	case DOC:
		amx_doc(a, *reg);
		return;
	case SKPBN:
		break;
	case SKPDN:
	case SKPBZ:
		npc++;
		break;
	default:
		fprintf(stderr, "AMX%o: ioi=0x%04x reg=0x%04x\n",
		    a->dev, ioi, *reg);
		exit(1);
		return;
	}
}

static struct amx *
NewAmx(int dev)
{
	struct amx *a;
	unsigned u;
	char buf[64];
	struct termios t;
	
	a = calloc(sizeof *a, 1);
	a->dev = dev;
	for (u = 0; u < 8; u++) {
		a->ch[u].fd = -1;
	}
		
	for (u = 0; u < 1; u++) {
		sprintf(buf, "/dev/nmdm%uA", u);
		a->ch[u].fd = open(buf, O_RDWR);
		if (a->ch[u].fd < 0) {
			perror(buf);
			exit(1);
		}
		tcgetattr(a->ch[u].fd, &t);
		cfmakeraw(&t);
		tcsetattr(a->ch[u].fd, TCSANOW, &t);
		assert(a->ch[u].fd >= 0);
		a->ch[u].cd = 1;
		a->ch[u].dsr = 1;
	}
	return (a);
}

int
config_amx(char **ap)
{
	unsigned d = 42;

	if (strcmp(ap[0], "amx"))
		return (0);
	if (!strcmp(ap[1], "config")) {
		if (ap[2] != NULL)
			d = strtoul(ap[2], NULL, 0);
		iodevs[d].func = dev_amx;
		iodevs[d].imask = 2;
		iodevs[d].priv = NewAmx(d);
		timeout(100000000, AMX_Timeout, iodevs[d].priv, 0);
		return (1);
	}
	return (0);
}
