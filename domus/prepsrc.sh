#!/bin/sh
#
# Eliminate unnecessary bytes from DOMAC source.
# Also: s/NL/CRNL/g

sed '
s/;.*//
/^[ 	]*$/d
s/[ 	][ 	]*/ /g
s/^ //
s/$//
'
 
