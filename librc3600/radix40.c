#include <stdio.h>
#include <sys/types.h>

#include <rc3600.h>
#include <domusobj.h>

static const char rad40[40] = ".0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char *
Radix40(uint16_t u, uint16_t v, char *p)
{
	static char buf[6];

	if (p == NULL)
		p = buf;

	p[2] = rad40[u % 40];
	u /= 40;
	p[1] = rad40[u % 40];
	u /= 40;
	p[0] = rad40[u % 40];
	v /= 32;
	p[4] = rad40[v % 40];
	v /= 40;
	p[3] = rad40[v % 40];
	p[5] = '\0';
	return (p);
}
