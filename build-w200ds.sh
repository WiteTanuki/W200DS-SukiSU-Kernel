#!/usr/bin/env bash
set -euo pipefail

repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
out=${OUT:-"$repo/out/w200ds-r11"}
jobs=${JOBS:-8}
clang_bin=${CLANG_BIN:?set CLANG_BIN to clang-r416183b/bin}
config="$repo/arch/arm64/configs/w200ds_r11_defconfig"
expected_config=f65e65f067d753eba838a479e075b91feb5f87027e191f58cd51e6e7e1501a77
expected_image=SET_AFTER_V0_2_0_RC1_BUILD

sha256() { sha256sum "$1" | awk '{print $1}'; }
[[ -x "$clang_bin/clang" ]] || { echo "missing clang: $clang_bin/clang" >&2; exit 1; }
"$clang_bin/clang" --version | grep -F 'r416183b' >/dev/null
[[ $(sha256 "$config") == "$expected_config" ]]
[[ $(cat "$repo/.scmversion") == '-android12-9-g83f519fbd509' ]]

mkdir -p "$out"
install -m 0644 "$config" "$out/.config"
export PATH="$clang_bin:$PATH"
export KBUILD_BUILD_USER=release
export KBUILD_BUILD_HOST=w200ds
export KBUILD_BUILD_TIMESTAMP=@1788192000
export KBUILD_BUILD_VERSION=1
export SOURCE_DATE_EPOCH=1788192000
export KCFLAGS="-fdebug-prefix-map=$repo=/usr/src/w200ds-kernel -ffile-prefix-map=$repo=/usr/src/w200ds-kernel -fmacro-prefix-map=$repo=/usr/src/w200ds-kernel -fdebug-prefix-map=$out=/usr/src/w200ds-kernel-out -ffile-prefix-map=$out=/usr/src/w200ds-kernel-out -fmacro-prefix-map=$out=/usr/src/w200ds-kernel-out"
export KAFLAGS="$KCFLAGS"
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
