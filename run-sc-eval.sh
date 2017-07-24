#!/bin/bash

#CHECKSUM_BIN=/usr/local/bin
CHECKSUM_BIN=build/src

binary=$1
connectivity=$2
module=$3
suffix="_modified"

$CHECKSUM_BIN/self-checksumming $binary $connectivity $module
python modify.py $binary$suffix
