#!/bin/sh

DUMPID=`head -n1 $1 | cut -f4 -d' '`
mkdir -p $2/$DUMPID
cp $1 $2/$DUMPID/
