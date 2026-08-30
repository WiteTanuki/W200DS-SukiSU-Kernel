#!/usr/bin/env bash
set -euo pipefail

repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${OUT:-"$repo/out/w200ds-r11"}
jobs=${JOBS:-8}
clang_bin=${CLANG_BIN:?set CLANG_BIN to clang-r416183b/bin}
config="$repo/arch/arm64/configs/w200ds_r11_defconfig"
expected_config=f0a99476257ad85eaf5669a9dc4e082fca4435c8e8c7aea6aca83cb1213ff897
expected_image=ec46347cfc1ad9b2ce765d010afd0bf88ab742e888ec4568507aaf39a4cfafb0

sha256() { sha256sum "$1" | awk '{print $1}'; }
[[ -x "$clang_bin/clang" ]] || { echo "missing clang: $clang_bin/clang" >&2; exit 1; }
"$clang_bin/clang" --version | grep -F 'r416183b' >/dev/null
[[ $(sha256 "$config") == "$expected_config" ]]
[[ $(cat "$repo/.scmversion") == '-android12-9-g83f519fbd509' ]]

mkdir -p "$out"
install -m 0644 "$config" "$out/.config"
export PATH="$clang_bin:$PATH"
common=( -C "$repo" O="$out" ARCH=arm64 LLVM=1 LLVM_IAS=1
  CROSS_COMPILE=aarch64-linux-gnu-
  CROSS_COMPILE_COMPAT=arm-linux-androidkernel- )
make "${common[@]}" olddefconfig
[[ $(sha256 "$out/.config") == "$expected_config" ]]
make "${common[@]}" -j"$jobs" Image modules

image="$out/arch/arm64/boot/Image"
[[ $(sha256 "$image") == "$expected_image" ]]
echo "IMAGE=$image"
echo "IMAGE_SHA256=$(sha256 "$image")"
echo "KERNELRELEASE=$(make -s "${common[@]}" kernelrelease)"

