#!/bin/bash

#CHECKSUM_BIN=/usr/local/bin
CHECKSUM_BIN=build/src

binary=$1
connectivity=$2
module=$3

$CHECKSUM_BIN/self-checksumming $binary $connectivity $module

