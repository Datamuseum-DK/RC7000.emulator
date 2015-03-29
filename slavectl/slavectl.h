
#define AZ(foo)         do { assert((foo) == 0); } while (0)
#define AN(foo)         do { assert((foo) != 0); } while (0)

int DoCmd(uint16_t cmd, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3,
    uint16_t *upload, uint16_t uplen, uint16_t *download, uint16_t downlen);

extern unsigned long rx_count;
extern unsigned long tx_count;
extern unsigned long cmd_count;


/* slavectl_dkp.c */
void DKP_smartdownload(const char *fn);

/* slavectl_mem.c */
uint16_t FreeMem(void);
uint16_t ChkSum(uint16_t from, uint16_t len);
void Upload(uint16_t dst, uint16_t *fm, uint16_t len);
void Download(uint16_t src, uint16_t len, uint16_t *to);
void Fill(uint16_t from, uint16_t to, uint16_t val);
uint16_t Compare(uint16_t src1, uint16_t src2, uint16_t len);
void Chew(uint16_t src, uint16_t buflen, uint16_t bufcnt, uint16_t *res);
uint16_t Move(uint16_t src, uint16_t dst, uint16_t len);
void DownloadAndMove(uint16_t src, uint16_t len, uint16_t dst, uint16_t *to);
