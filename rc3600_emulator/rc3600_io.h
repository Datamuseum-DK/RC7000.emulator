/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 */

#ifndef _RC3600_IO_H_
#define _RC3600_IO_H_

#include "rc3600_emul.h"

typedef void event_f(void *, int);
typedef int ioconf_f(char **ap);
typedef void trace_f(uint16_t xpc, int type, u_int *next_event);

/************************************************************************/
#define IO_OPER(x)	((x) & ~0x00c0)

/************************************************************************/

#define NSEC 1000000000U

/* rc3600.c */
void		ioprint(const char *fmt, ...);
void		irq_lower(struct iodev *io);
void		timeout(unsigned count, event_f func, void *arg1, int arg2);
event_f		dev_irq;
uint16_t	CR(uint16_t a);
void		CW(uint16_t a, uint16_t d);
void		trace_set(uint16_t adr, trace_f *func);
void		trace_reset(uint16_t adr, trace_f *func);

/* io_dkp.c */
ioconf_f	config_dkp;

/* io_stat.c */
ioconf_f	config_stat;
extern uint64_t NINS, NRD, NWR;

/* io_floppy.c */
ioconf_f	config_floppy;

/* io_lpt.c */
ioconf_f	config_lpt;

/* io_ptp.c */
ioconf_f	config_ptp;

/* io_ptr.c */
ioconf_f	config_ptr;

/* io_amx.c */
ioconf_f	config_amx;

/* io_rc3751.c */
ioconf_f	config_rc3751;

/* io_rtc.c */
ioconf_f	config_rtc;

/* io_tty.c */
ioconf_f	config_tty;
int		TTYI_Input(int chr);

/* os_domus.c */
ioconf_f	config_domus;

#endif /* _RC3600_IO_H_ */
