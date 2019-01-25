#!/bin/sh

testdir=''

args=$(getopt d: $*)
if [ $? -ne 0 ]; then
    echo >&2 "Usage: $0 ....";
    exit 2
fi
set -- $args
while [ $# -gt 0 ]; do
	case $1 in
	-d) testdir=$2; shift;;
	--) shift; break;;
	esac
	shift
done


: ${TMP:=/tmp}

if [ -n "$testdir" ]; then
    [ -d $testdir ] || mkdir $testdir
else
    testdir=${TMP}/testdir.$$
    mkdir $testdir
    trap "rm -rf $testdir" 0 1 2 3 15
fi

img1=$testdir/1.ppm
img2=$testdir/2.ppm
img3=$testdir/3.ppm
img4=$testdir/4.ppm

ppmmake red 10 20 > $img1
ppmmake blue 30 30 > $img2

ppmmake green 100 100 | pnmpaste -replace $img1 10 5 | pnmpaste -replace $img2 50 50 > $img3
ppmmake green 100 100 | ../pnmpaste -replace $img1 10 5 | ../pnmpaste -replace $img2 50 50 > $img4

cmp $img3 $img4 || exit 1

ppmmake green 100 100 | ../pnmpaste_multi -replace $img1 10 5 $img2 50 50 > $img4
cmp $img3 $img4 || exit 1
