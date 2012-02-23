
/* disass.c */
struct disass_magic {
	uint16_t	n;
	const char	*s;
};

#define Rc3600Disass_NO_OFFSET	-9999

char *Rc3600Disass(uint16_t u, struct disass_magic *magic, const char **pz, const char **iodev, char *buf, int *offset);


/* disass_domus.c */
char *Domus2Disass(uint16_t u, char *buf, int *offset);
char *Domus3Disass(uint16_t u, char *buf, int *offset);

/* radix40.c */
char *Radix40(uint16_t u, uint16_t v, char *p);
