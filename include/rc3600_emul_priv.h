/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $Id: rc3600_emul_priv.h,v 1.1 2007/02/25 17:43:03 phk Exp $
 */

enum timing {
	TIME_LDA,
	TIME_STA,
	TIME_ISZ,
	TIME_ISZ_SKP,
	TIME_JMP,
	TIME_JSR,
	TIME_INDIR_ADR,
	TIME_BASE_REG,
	TIME_AUTO_IDX,
	TIME_ALU_1,
	TIME_ALU_2,
	TIME_ALU_SKIP,
	TIME_IO_INPUT,
	TIME_IO_NIO,
	TIME_IO_OUTPUT,
	TIME_IO_SCP,
	TIME_IO_SKP,
	TIME_IO_SKP_SKIP,
	TIME_IO_INTA,
	TIME_LASTONE		/* marker */
};
