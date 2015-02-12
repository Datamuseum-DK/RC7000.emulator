#include <sys/queue.h>

#define WVMASK  0x0000ffff
#define WOFLOW  0x00010000
#define WRSHIFT 17
#define WRMASK  (7 << WRSHIFT)
#define WVALID  0x00100000
#define WMSG    0x80000000
#define WZONE   0x40000000
#define WDEFCUR (0x20000000 | WVALID)
        
#define RABS            1
#define RNORM           2
#define RBYTE           3
#define RHIGH           7

#define WZERO(foo)      do { (foo) = 0; } while (0)
#define WRELOC(foo)     (((foo) & WRMASK) >> WRSHIFT)
#define WADD(foo, bar)  do { (foo) = ((foo) + (bar)) & WVMASK; } while (0)
#define WVAL(foo)       ((foo) & WVMASK)
 
#define WISVALID(foo)   (foo & WVALID)
#define WISABS(foo)     (WRELOC(foo) == RABS)
#define WISBYTE(foo)    (WRELOC(foo) == RBYTE)

uint32_t Wtonorm(uint32_t a);
uint32_t Woffset(uint32_t a, int i);
const char *Wfmt(uint32_t w, char *buf);
void Wsetabs(uint32_t *w, unsigned val);

extern uint32_t CUR;

typedef int getc_f(void *priv);

#define	OBJNHDR		6			/* Constant constant */
#define	OBJNWORD	(OBJNHDR + 15 * 3)	/* Tunable constant */

struct domus_obj_rec {
	unsigned			nw;
	uint32_t			w[OBJNWORD];
	TAILQ_ENTRY(domus_obj_rec)	list;
};

struct domus_obj_obj {
	char				title[6];
	uint32_t			start;
	TAILQ_HEAD(,domus_obj_rec)	recs;
	TAILQ_ENTRY(domus_obj_obj)	list;
};

struct domus_obj_file {
	const char			*fn;
	int				in_leader;
	getc_f				*func;
	void				*priv;
	TAILQ_HEAD(,domus_obj_obj)	objs;
};


struct domus_obj_file *
ReadDomusObj(getc_f *f, void *priv, const char *fn, int verbose);

/* radix40.c */
char *Radix40(uint16_t u, uint16_t v, char *p);
