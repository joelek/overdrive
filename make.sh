#!/bin/sh

if [ $# -eq 1 ] && ([ $1 = "debug" ] || [ $1 = "release" ]); then
	echo "[building]"
else
	echo "usage: make <debug|release>"
	exit 1
fi

COMPILER_OPTIONS="-std=c++20 -static -pedantic -Wall -Wextra -O3"

if [ $1 = "debug" ]; then
	echo "[configuration: debug]"
	COMPILER_OPTIONS+=" -D DEBUG"
fi

if [ $1 = "release" ]; then
	echo "[configuration: release]"
	COMPILER_OPTIONS+=" -Werror -s"
fi

SOURCES=(
	"commands/cue"
	"commands/iso"
	"commands/mds"
	"lib/accuraterip"
	"lib/bcd"
	"lib/byteswap"
	"lib/cd"
	"lib/cdb"
	"lib/cdda"
	"lib/cdrom"
	"lib/cdxa"
	"lib/drive"
	"lib/enums"
	"lib/exceptions"
	"lib/idiv"
	"lib/iso9660"
	"lib/sense"
	"lib/string"
	"lib/type"
	"lib/wav"
	"lib"
)

TARGETS=(
	"main"
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
	echo "[compiling: source/$i.cpp]"
	mkdir -p $(dirname "build/objects/$i.o")
	gcc $COMPILER_OPTIONS -c source/$i.cpp -o build/objects/$i.o
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		echo "[failure]"
		exit 1;
	fi
done

echo "[phase: targets]"

for i in ${TARGETS[@]}; do
	echo "[compiling: source/$i.cpp]"
	mkdir -p $(dirname "build/targets/$i")
	gcc $COMPILER_OPTIONS ${OBJECTS[@]} source/$i.cpp -o build/targets/$i -l stdc++
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		echo "[failure]"
		exit 1;
	fi
done

echo "[success]"
