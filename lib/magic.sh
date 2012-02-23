#!/bin/sh

(
echo 'struct magic magic[] = {'
(
sed '
/ 0C00 /d
/JSR/!d
s/^.... */	{ 0x/
s/ .DUSR /, "/
s/ *=.*/" },/
' mupar.list

sed '
/ 0400 /d
/JMP/!d
s/^.... */	{ 0x/
s/ .DUSR /, "/
s/ *=.*/" },/
' mupar.list

echo '
	{ 0x607F, "INTEN" },	/* NIOS 0,CPU */
	{ 0x60BF, "INTDS" },	/* NIOC 0,CPU */
	{ 0x643F, "MSKO" },	/* DOB  0,CPU */
	{ 0x65BF, "IORST" },	/* DICC 0,CPU */
	{ 0x663F, "HALT" },	/* DOC  0,CPU */
	{ 0x6b3F, "INTA  1" },	/* DIB  1,CPU */

	{ 0x0C9E, "TAKEA" },	/* Interpreter takeaddress ()*/
	{ 0x0C9F, "TAKEV" },	/* Interpreter takevalue() */
	{ 0x049D, "INEXT" },	/* Interpreter nextstep */
'
) | sort
echo '	{ 0, NULL }'
echo '};'
)
