#!/bin/bash
set -eu
./build.sh
FOLDER_NAME=${PWD##*/}
./"$FOLDER_NAME.bin" $@
