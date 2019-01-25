#!/bin/sh

#
# PBM format
#

: ${TMP:=/tmp}

TD=$TMP/testdir.$$
#TD=./tst

mkdir $TD

img() {
    echo "$TD/img$1.pbm"
}

trap "rm -rf $TD" 0 1 2 3 15

pbmtext ABC > $(img 1)
pnminvert $(img 1) > $(img 2)
pbmtext DE > $(img 3)
pnminvert $(img 3) > $(img 4)

pbmmake -black 100 100 | pnmpaste -or $(img 2) 10 5 | pnmpaste -or $(img 4) 16 10 > $(img 5)
pbmmake -black 100 100 | ../pnmpaste -or $(img 2) 10 5 | ../pnmpaste -or $(img 4) 16 10 > $(img 6)

cmp $(img 5) $(img 6) || exit 1

pbmmake -black 100 100 | ../pnmpaste_multi -or $(img 2) 10 5 $(img 4) 16 10 > $(img 6)
cmp $(img 5) $(img 6) || exit 1

pbmmake -black 100 100 | pnmpaste -replace $(img 2) 10 5 | pnmpaste -replace $(img 4) 16 20 > $(img 5)
pbmmake -black 100 100 | ../pnmpaste -replace $(img 2) 10 5 | ../pnmpaste -replace $(img 4) 16 20 > $(img 6)

cmp $(img 5) $(img 6) || exit 1

pbmmake -black 100 100 | ../pnmpaste_multi -replace $(img 2) 10 5 $(img 4) 16 20 > $(img 6)
cmp $(img 5) $(img 6) || exit 1

pbmmake -white 100 100 | pnmpaste -and $(img 1) 10 5 | pnmpaste -and $(img 3) 16 10 > $(img 5)
pbmmake -white 100 100 | ../pnmpaste -and $(img 1) 10 5 | ../pnmpaste -and $(img 3) 16 10 > $(img 6)

cmp $(img 5) $(img 6) || exit 1

pbmmake -white 100 100 | ../pnmpaste_multi -and $(img 1) 10 5 $(img 3) 16 10 > $(img 6)
cmp $(img 5) $(img 6) || exit 1


pbmmake -white 100 100 | pnmpaste -xor $(img 1) 10 5 | pnmpaste -xor $(img 3) 16 10 > $(img 5)
pbmmake -white 100 100 | ../pnmpaste -xor $(img 1) 10 5 | ../pnmpaste -xor $(img 3) 16 10 > $(img 6)

cmp $(img 5) $(img 6) || exit 1

pbmmake -white 100 100 | ../pnmpaste_multi -xor $(img 1) 10 5 $(img 3) 16 10 > $(img 6)
cmp $(img 5) $(img 6) || exit 1

