#!/bin/bash

PA_FOLDER="PortAudio"
PA_URL="https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz"
CURRENT_DIR=$(pwd)
PA_ARCHIVE="pa_stable_v190700_20210406.tgz"
OUT_FILE="lib/.lib/libportaudio.a"
echo "$CURRENT_DIR"
mkdir -p "$PA_FOLDER"
cd "$PA_FOLDER"

#   Download the source
wget $PA_URL && echo "$(ls -a)"
#   Untar it
tar -xzf "$PA_ARCHIVE"
cd portaudio
#   Configure and install
./configure && make
#   Copy to the destination folder
cp $OUT_FILE "$CURRENT_DIR"

