#!/usr/bin/env bash
set -euo pipefail

repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
runtime="$repo/tools/kernelpatch-runtime"
output=${1:?usage: $0 OUTPUT_DIR}
ndk=${ANDROID_NDK_HOME:?set ANDROID_NDK_HOME to Android NDK r29}
expected_kpimg=34bab04a68b28fae689d144ad1245c9271cf53a172068340050adf160211cbbb
expected_elf=eb8251bbbec7c3713b0d7dd321a2be1b8924a85e9a4f30420f401e420d40bb44
expected_tools=c6fd32e3235166efc2f5bc784b1486154d16299da550cf4d4a339a960f484cae

grep -qx 'Pkg.Revision = 29.0.14206865' "$ndk/source.properties"
[[ ! -e "$output/kpimg" && ! -e "$output/kpimg.elf" && ! -e "$output/kptools" ]]
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cp -a "$runtime/." "$stage/runtime/"

export SOURCE_DATE_EPOCH=1787969194 TZ=UTC LC_ALL=C
ANDROID_NDK_HOME="$ndk" make -C "$stage/runtime/kernel" clean all
install -m 0644 "$stage/runtime/kernel/include/preset.h" "$stage/runtime/tools/preset.h"
make -C "$stage/runtime/tools" CC=gcc
mkdir -p "$output"
install -m 0644 "$stage/runtime/kernel/kpimg" "$output/kpimg"
install -m 0644 "$stage/runtime/kernel/kpimg.elf" "$output/kpimg.elf"
install -m 0755 "$stage/runtime/tools/kptools" "$output/kptools"

check() { [[ $(sha256sum "$1" | awk '{print $1}') == "$2" ]]; }
check "$output/kpimg" "$expected_kpimg"
check "$output/kpimg.elf" "$expected_elf"
check "$output/kptools" "$expected_tools"
sha256sum "$output/kpimg" "$output/kpimg.elf" "$output/kptools"
