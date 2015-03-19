
#define AZ(foo)         do { assert((foo) == 0); } while (0)
#define AN(foo)         do { assert((foo) != 0); } while (0)

void PW(uint16_t u);
uint16_t GW(void);

int DoCmd(uint16_t cmd, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3,
    uint16_t *upload, uint16_t uplen, uint16_t *download, uint16_t downlen);

uint16_t ChkSum(uint16_t from, uint16_t len);
void Upload(uint16_t dst, uint16_t *fm, uint16_t len);
void Download(uint16_t src, uint16_t len, uint16_t *to);
void Fill(uint16_t from, uint16_t to, uint16_t val);

/* slavectl_dkp.c */
void DKP_download(const char *fn);
