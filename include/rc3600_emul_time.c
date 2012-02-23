/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $Id: rc3600_emul_time.c,v 1.2 2007/02/25 20:12:36 phk Exp $
 *
 * Data from: 015-000023-01_ProgRef.pdf, page F - 1 of 4 (pdf page 67)
 */

#include "rc3600_emul_priv.h"

#ifndef NS2X
#define NS2X(x)	(x)
#endif

unsigned nova_timing[] = {
	[TIME_LDA] =		NS2X(5200),
	[TIME_STA] =		NS2X(5500),
	[TIME_ISZ] =		NS2X(5200),
	[TIME_JMP] =		NS2X(5600),
	[TIME_JSR] =		NS2X(3500),
	[TIME_INDIR_ADR] =	NS2X(2600),
	[TIME_BASE_REG] =	NS2X(300),
	[TIME_ALU_1] =		NS2X(5600),
	[TIME_ALU_2] =		NS2X(5900),
	[TIME_IO_INPUT] =	NS2X(4400),
	[TIME_IO_NIO] =		NS2X(4400),
	[TIME_IO_OUTPUT] =	NS2X(4700),
	[TIME_IO_SKP] =		NS2X(4400),
	[TIME_IO_INTA] =	NS2X(4400),
};

unsigned nova1200_timing[] = {
	[TIME_LDA] =		NS2X(2550),
	[TIME_STA] =		NS2X(2550),
	[TIME_ISZ] =		NS2X(3150),
	[TIME_ISZ_SKP] =	NS2X(1350),
	[TIME_JMP] =		NS2X(1350),
	[TIME_JSR] =		NS2X(1350),
	[TIME_INDIR_ADR] =	NS2X(1200),
	[TIME_AUTO_IDX] =	NS2X(600),
	[TIME_ALU_1] =		NS2X(1350),
	[TIME_ALU_2] =		NS2X(1350),
	[TIME_ALU_SKIP] =	NS2X(1350),
	[TIME_IO_INPUT] =	NS2X(2550),
	[TIME_IO_NIO] =		NS2X(3150),
	[TIME_IO_OUTPUT] =	NS2X(3150),
	[TIME_IO_SKP] =		NS2X(2550),
	[TIME_IO_INTA] =	NS2X(2550),
};

unsigned nova800_timing[] = {
	[TIME_LDA] =		NS2X(1600),
	[TIME_STA] =		NS2X(1600),
	[TIME_ISZ] =		NS2X(1800),
	[TIME_JMP] =		NS2X(800),
	[TIME_JSR] =		NS2X(800),
	[TIME_INDIR_ADR] =	NS2X(800),
	[TIME_AUTO_IDX] =	NS2X(200),
	[TIME_ALU_1] =		NS2X(800),
	[TIME_ALU_2] =		NS2X(800),
	[TIME_ALU_SKIP] =	NS2X(200),
	[TIME_IO_INPUT] =	NS2X(2200),
	[TIME_IO_NIO] =		NS2X(2200),
	[TIME_IO_OUTPUT] =	NS2X(2200),
	[TIME_IO_SCP] =		NS2X(600),
	[TIME_IO_SKP] =		NS2X(1400),
	[TIME_IO_SKP_SKIP] =	NS2X(200),
	[TIME_IO_INTA] =	NS2X(2200),
};

unsigned nova2_timing[] = {
	[TIME_LDA] =		NS2X(2000),
	[TIME_STA] =		NS2X(2000),
	[TIME_ISZ] =		NS2X(2100),
	[TIME_JMP] =		NS2X(1000),
	[TIME_JSR] =		NS2X(1200),
	[TIME_INDIR_ADR] =	NS2X(1000),
	[TIME_AUTO_IDX] =	NS2X(500),
	[TIME_ALU_1] =		NS2X(1000),
	[TIME_ALU_2] =		NS2X(1000),
	[TIME_ALU_SKIP] =	NS2X(200),
	[TIME_IO_INPUT] =	NS2X(1500),
	[TIME_IO_NIO] =		NS2X(1700),
	[TIME_IO_OUTPUT] =	NS2X(1700),
	[TIME_IO_SKP] =		NS2X(1200),
	[TIME_IO_SKP_SKIP] =	NS2X(200),
	[TIME_IO_SCP] =		NS2X(300),
	[TIME_IO_INTA] =	NS2X(1500),
};

