#!/bin/bash

CHECKSUM_BIN=/usr/local/bin

binary=$1
connectivity=$2
module=$3

$CHECKSUM_BIN/self-checksumming $binary $connectivity $module

