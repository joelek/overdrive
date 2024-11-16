#!/bin/sh

if [ $# -eq 1 ] && ([ $1 = "debug" ] || [ $1 = "release" ]); then
	echo "[building]"
else
	echo "usage: make <debug|release>"
	exit 1
fi

COMPILER_OPTIONS="-std=c++20 -static -pedantic -Wall -Wextra -Werror=return-type -O3"

if [ $1 = "debug" ]; then
	echo "[configuration: debug]"
	COMPILER_OPTIONS+=" -D DEBUG"
fi

if [ $1 = "release" ]; then
	echo "[configuration: release]"
	COMPILER_OPTIONS+=" -Werror -s"
fi

SOURCES=(
	"cli/commands/cue.cpp"
	"cli/commands/iso.cpp"
	"cli/commands/mds.cpp"
	"cli/commands/odi.cpp"
	"lib/accuraterip.cpp"
	"lib/app.cpp"
	"lib/bcd.cpp"
	"lib/bits.cpp"
	"lib/byteswap.cpp"
	"lib/cd.cpp"
	"lib/cdb.cpp"
	"lib/cdda.cpp"
	"lib/cdrom.cpp"
	"lib/cdxa.cpp"
	"lib/command.cpp"
	"lib/copier.cpp"
	"lib/crc.cpp"
	"lib/detail.cpp"
	"lib/disc.cpp"
	"lib/drive.cpp"
	"lib/endian.cpp"
	"lib/exceptions.cpp"
	"lib/idiv.cpp"
	"lib/iso9660.cpp"
	"lib/logger.cpp"
	"lib/mds.cpp"
	"lib/memory.cpp"
	"lib/odi.cpp"
	"lib/options.cpp"
	"lib/overdrive.cpp"
	"lib/path.cpp"
	"lib/parser.cpp"
	"lib/scsi.cpp"
	"lib/sense.cpp"
	"lib/shared.cpp"
	"lib/string.cpp"
	"lib/time.cpp"
	"lib/vector.cpp"
	"lib/wav.cpp"
)

TARGETS=(
	"cli/overdrive.cpp"
)

mkdir -p build/objects
mkdir -p build/targets

rm -rf build/objects/*
rm -rf build/targets/*

OBJECTS=()

for i in ${SOURCES[@]}; do
	OBJECTS+=" build/objects/$i.o"
done

echo "[phase: sources]"

for i in ${SOURCES[@]}; do
	echo "[compiling: source/$i]"
	mkdir -p $(dirname "build/objects/$i.o")
	gcc $COMPILER_OPTIONS -c source/$i -o build/objects/$i.o
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		echo "[failure]"
		exit 1;
	fi
done

echo "[phase: targets]"

for i in ${TARGETS[@]}; do
	echo "[compiling: source/$i]"
	mkdir -p $(dirname "build/targets/$i")
	gcc $COMPILER_OPTIONS ${OBJECTS[@]} source/$i -o build/targets/${i%.*} -l stdc++
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		echo "[failure]"
		exit 1;
	fi
done

echo "[success]"
