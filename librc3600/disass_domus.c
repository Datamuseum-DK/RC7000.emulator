#include <stdio.h>
#include <sys/types.h>
#include <rc3600.h>


static const char *iodev[64] = {
	"00",	"01",	"02",	"03",	"04",	"05",	"06",	"07",
	"TTYI",	"TTYO",	"PTR",	"PTP",	"RTC",	"0d",	"0e",	"0f",
	"10",	"11",	"12",	"13",	"14",	"15",	"16",	"17",
	"18",	"19",	"1a",	"1b",	"1c",	"1d",	"1e",	"1f",
	"20",	"21",	"22",	"23",	"24",	"25",	"26",	"27",
	"28",	"29",	"AMX",	"2b",	"2c",	"2d",	"2e",	"2f",
	"30",	"FD0",	"32",	"33",	"FD1",	"35",	"36",	"37",
	"38",	"39",	"3a",	"DKP",	"3c",	"3d",	"3e",	"CPU",
};

static struct disass_magic magic[] = {

	{ 0x0474, ".NEXTOPERATION" },
	{ 0x0475, ".RETURNANSWER" },
	{ 0x0476, ".CLEARDEVICE" },
	{ 0x0478, ".SETINTERRUPT" },
	{ 0x0479, ".SETRESERVATION" },
	{ 0x047A, ".SETCONVERSION" },
	{ 0x047B, ".CONBYTE" },
	{ 0x047C, ".GETBYTE" },
	{ 0x047D, ".PUTBYTE" },
	{ 0x047E, ".MULTIPLY" },
	{ 0x047F, ".DIVIDE" },
	{ 0x0480, ".GETREC" },
	{ 0x0481, ".PUTREC" },
	{ 0x0482, ".WAITTRANSFER" },
	{ 0x0483, ".REPEATSHARE" },
	{ 0x0484, ".TRANSFER" },
	{ 0x0485, ".INBLOCK" },
	{ 0x0486, ".OUTBLOCK" },
	{ 0x0487, ".INCHAR" },
	{ 0x0488, ".FREESHARE" },
	{ 0x0489, ".OUTSPACE" },
	{ 0x048A, ".OUTCHAR" },
	{ 0x048B, ".OUTNL" },
	{ 0x048C, ".OUTEND" },
	{ 0x048D, ".OUTTEXT" },
	{ 0x048E, ".OUTOCTAL" },
	{ 0x048F, ".SETPOSITION" },
	{ 0x0490, ".CLOSE" },
	{ 0x0491, ".OPEN" },
	{ 0x049D, "INEXT" },	/* Interpreter nextstep */
	{ 0x0C02, "WAIT" },
	{ 0x0C03, "WAITINTERRUPT" },
	{ 0x0C04, "SENDMESSAGE" },
	{ 0x0C05, "WAITANSWER" },
	{ 0x0C06, "WAITEVENT" },
	{ 0x0C07, "SENDANSWER" },
	{ 0x0C08, "SEARCHITEM" },
	{ 0x0C09, "CLEANPROCESS" },
	{ 0x0C0A, "BREAKPROCESS" },
	{ 0x0C0B, "STOPPROCESS" },
	{ 0x0C0C, "STARTPROCESS" },
	{ 0x0C0D, "RECHAIN" },
	{ 0x0C74, "NEXTOPERATION" },
	{ 0x0C75, "RETURNANSWER" },
	{ 0x0C77, "WAITOPERATION" },
	{ 0x0C78, "SETINTERRUPT" },
	{ 0x0C79, "SETRESERVATION" },
	{ 0x0C7A, "SETCONVERSION" },
	{ 0x0C7B, "CONBYTE" },
	{ 0x0C7C, "GETBYTE" },
	{ 0x0C7D, "PUTBYTE" },
	{ 0x0C7E, "MULTIPLY" },
	{ 0x0C7F, "DIVIDE" },
	{ 0x0C80, "GETREC" },
	{ 0x0C81, "PUTREC" },
	{ 0x0C82, "WAITTRANSFER" },
	{ 0x0C84, "TRANSFER" },
	{ 0x0C85, "INBLOCK" },
	{ 0x0C86, "OUTBLOCK" },
	{ 0x0C87, "INCHAR" },
	{ 0x0C88, "FREESHARE" },
	{ 0x0C89, "OUTSPACE" },
	{ 0x0C8A, "OUTCHAR" },
	{ 0x0C8B, "OUTNL" },
	{ 0x0C8C, "OUTEND" },
	{ 0x0C8D, "OUTTEXT" },
	{ 0x0C8E, "OUTOCTAL" },
	{ 0x0C8F, "SETPOSITION" },
	{ 0x0C90, "CLOSE" },
	{ 0x0C91, "OPEN" },
	{ 0x0C92, "WAITZONE" },
	{ 0x0C93, "INNAME" },
	{ 0x0C94, "MOVE" },
	{ 0x0C95, "INTPRETE" },
	{ 0x0C9A, "BINDEC" },
	{ 0x0C9B, "DECBIN" },
	{ 0x0C9E, "TAKEA" },	/* Interpreter takeaddress ()*/
	{ 0x0C9F, "TAKEV" },	/* Interpreter takevalue() */
	{ 0x0CDA, "NEWCAT" },
	{ 0x0CDB, "FREECAT" },
	{ 0x0CDC, "CDELAY" },
	{ 0x0CDD, "WAITSE" },
	{ 0x0CDE, "WAITCH" },
	{ 0x0CDF, "CWANSW" },
	{ 0x0CE0, "CTEST" },
	{ 0x0CE1, "CPRINT" },
	{ 0x0CE2, "CTOUT" },
	{ 0x0CE3, "SIGNAL" },
	{ 0x0CE4, "SIGCH" },
	{ 0x0CE5, "CPASS" },
	{ 0x0CE6, "CREATEENTRY" },
	{ 0x0CE7, "LOOKUPENTRY" },
	{ 0x0CE8, "CHANGEENTRY" },
	{ 0x0CE9, "REMOVEENTRY" },
	{ 0x0CEA, "INITCATALOG" },
	{ 0x0CEB, "SETENTRY" },
	{ 0x0CEC, "COMON" },
	{ 0x0CED, "CALL" },
	{ 0x0CEE, "GOTO" },
	{ 0x0CEF, "GETADR" },
	{ 0x0CF0, "GETPOINT" },
	{ 0x0CF4, "CSENDM" },
	{ 0x0CF5, "SIGGEN" },
	{ 0x0CF6, "WAITGE" },
	{ 0x0CF7, "CTOP" },
	{ 0x607F, "INTEN" },	/* NIOS 0,CPU */
	{ 0x60BF, "INTDS" },	/* NIOC 0,CPU */
	{ 0x643F, "MSKO" },	/* DOB  0,CPU */
	{ 0x65BF, "IORST" },	/* DICC 0,CPU */
	{ 0x663F, "HALT" },	/* DOC  0,CPU */
	{ 0x6b3F, "INTA  1" },	/* DIB  1,CPU */
	{ 0, NULL }
};

static const char *PZ[256] = {
/* 00 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 10 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 20 */ "CUR",	   NULL,     NULL,     NULL,	 NULL,	   "TABLE",  "TOPTA", NULL,
	 NULL,	   NULL,     "PFIRS",  NULL,	 "RUNNI",  ".0",     "EXIT",  "EFIRS",
/* 30 */ "FFIRS",  "DELAY",  NULL,     NULL,	 "AREAP",  "AFIRS",  "FREQ",  "MASK",
	 "CORES",  "PROGR",  NULL,     NULL,     "RTIME",  "RTIME+1","POWIN",  NULL,
/* 40 */ "CPUTY",  ".32768", ".16384", ".8192",  ".4096",  ".2048",  ".1024",  ".512",
	 ".256",   ".128",   ".64",    ".32",    ".16",    ".8",     ".4",     ".2",
/* 50 */ ".1",     ".3",     ".5",     ".6",	 ".7",	   ".9",     ".10",    ".12",
	 ".13",    ".15",    ".24",    ".25",	 ".40",	   ".48",    ".56",    ".60",
/* 60 */	".63",	".120",	".127",	".255",	".M3",	".M4",	".M16",	".M256",
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 70 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 80 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* 90 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* a0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* b0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* c0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* d0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* e0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
/* f0 */	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
		NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,	NULL,
};

char *Domus2Disass(uint16_t u, char *buf, int *offset)
{

	return (Rc3600Disass(
	    u, magic, PZ, iodev, buf, offset));
}

char *Domus3Disass(uint16_t u, char *buf, int *offset)
{

	return (Rc3600Disass(
	    u, magic, PZ, iodev, buf, offset));
}

