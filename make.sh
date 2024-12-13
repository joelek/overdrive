#!/bin/sh

source ./version.env

APP_VERSION="$MAJOR.$MINOR.$PATCH"

if [ $# -eq 1 ] && ([ $1 = "debug" ] || [ $1 = "release" ]); then
	echo "[building: $APP_VERSION-$1]"
else
	echo "usage: make <debug|release>"
	exit 1
fi

COMPILER="g++"
COMPILER_OPTIONS=(
	"-std=c++20"
	"-pedantic"
	"-Wall"
	"-Wextra"
	"-Werror=return-type"
	"-O3"
	"-D APP_VERSION=\"$APP_VERSION\""
)

LINKER="g++"
LINKER_OPTIONS=(
	"-static"
	"-l stdc++"
)

if [ $1 = "debug" ]; then
	COMPILER_OPTIONS+=(
		"-D DEBUG"
	)
fi

if [ $1 = "release" ]; then
	COMPILER_OPTIONS+=(
		"-Werror"
	)
	LINKER_OPTIONS+=(
		"-s"
	)
fi

SOURCES=(
	"cli/tasks/cue.cpp"
	"cli/tasks/iso.cpp"
	"cli/tasks/mds.cpp"
	"cli/tasks/odi.cpp"
	"lib/accuraterip.cpp"
	"lib/archiver.cpp"
	"lib/bcd.cpp"
	"lib/bits.cpp"
	"lib/byteswap.cpp"
	"lib/cd.cpp"
	"lib/cdb.cpp"
	"lib/cdda.cpp"
	"lib/cdrom.cpp"
	"lib/cdxa.cpp"
	"lib/crc.cpp"
	"lib/detail.cpp"
	"lib/disc.cpp"
	"lib/drive.cpp"
	"lib/emulator.cpp"
	"lib/endian.cpp"
	"lib/exceptions.cpp"
	"lib/idiv.cpp"
	"lib/iso9660.cpp"
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
	"lib/task.cpp"
	"lib/time.cpp"
	"lib/vector.cpp"
	"lib/wav.cpp"
)

TARGETS=(
	"cli/overdrive.cpp"
)

rm -rf build/*

mkdir -p build/objects
mkdir -p build/targets

echo "[compiling]"
echo "[compiler: $COMPILER ${COMPILER_OPTIONS[@]}]"

for i in ${SOURCES[@]} ${TARGETS[@]}; do
	echo "[compiling: source/$i]"
	mkdir -p $(dirname "build/objects/$i.o")
	$COMPILER ${COMPILER_OPTIONS[@]} -c source/$i -o build/objects/$i.o
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		echo "[failure]"
		exit 1;
	fi
done

OBJECTS=()

for i in ${SOURCES[@]}; do
	OBJECTS+=(
		"build/objects/$i.o"
	)
done

echo "[linking]"
echo "[linker: $LINKER ${LINKER_OPTIONS[@]}]"

for i in ${TARGETS[@]}; do
	echo "[linking: source/$i]"
	mkdir -p $(dirname "build/targets/$i")
	$LINKER ${LINKER_OPTIONS[@]} ${OBJECTS[@]} build/objects/$i.o -o build/targets/${i%.*}
	RETURN_CODE=$?
	if [ $RETURN_CODE -gt 0 ]; then
		echo "[failure]"
		exit 1;
	fi
done

if [ $1 = "release" ]; then
	rm -rf dist/*
	mkdir -p dist
	cp -R "build/targets/." "dist"
fi

echo "[success]"
