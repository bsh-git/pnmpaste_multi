#!/bin/sh

: ${TMP:=/tmp}

img1=$TMP/1.ppm
img2=$TMP/2.ppm
img3=$TMP/3.ppm
img4=$TMP/4.ppm

trap 'rm -f $img1 $img2 $img3 $img4' 0 1 2 3 15

ppmmake red 10 20 > $img1
ppmmake blue 30 30 > $img2

ppmmake green 100 100 | pnmpaste -replace $img1 10 5 | pnmpaste -replace $img2 50 50 > $img3
ppmmake green 100 100 | ../pnmpaste -replace $img1 10 5 | ../pnmpaste -replace $img2 50 50 > $img4

cmp $img3 $img4 || exit 1

ppmmake green 100 100 | ../pnmpaste_multi -replace $img1 10 5 $img2 50 50 > $img4
cmp $img3 $img4 || exit 1
