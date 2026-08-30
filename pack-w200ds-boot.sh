#!/usr/bin/env bash
set -euo pipefail

stock=${1:?usage: $0 STOCK_BOOT MARKED_IMAGE MAGISKBOOT OUTPUT_BOOT}
image=${2:?usage: $0 STOCK_BOOT MARKED_IMAGE MAGISKBOOT OUTPUT_BOOT}
magiskboot=${3:?usage: $0 STOCK_BOOT MARKED_IMAGE MAGISKBOOT OUTPUT_BOOT}
output=${4:?usage: $0 STOCK_BOOT MARKED_IMAGE MAGISKBOOT OUTPUT_BOOT}
expected_stock=9ddb0f2b4bd2ce86cacad2d7091deb02edc87a564937e68560172b1f7877350e
expected_image=244e6dc3efab05ecbfc3de6ded31714a0cb87e2a7c7e02bcfaff1f1603f17e06
expected_magiskboot=a918c7033118829513c410af7eae6dd0a3c8fde97408b64f7a0bfd49aad3230b
expected_output=a0a52b34a1de45ae4dedf8298da65f87bb423e8f0eb78a7022ee427524210055
sha256() { sha256sum "$1" | awk '{print $1}'; }

[[ ! -e "$output" ]]
[[ $(sha256 "$stock") == "$expected_stock" && $(stat -c %s "$stock") -eq 67108864 ]]
[[ $(sha256 "$image") == "$expected_image" ]]
[[ $(sha256 "$magiskboot") == "$expected_magiskboot" && -x "$magiskboot" ]]
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir "$stage/stock" "$stage/check"
( cd "$stage/stock" && "$magiskboot" unpack -h "$stock" >/dev/null )
[[ -s "$stage/stock/kernel" && -s "$stage/stock/ramdisk.cpio" ]]
install -m 0644 "$image" "$stage/stock/kernel"
( cd "$stage/stock" && "$magiskboot" repack "$stock" "$stage/candidate.img" >/dev/null )
( cd "$stage/check" && "$magiskboot" unpack -h "$stage/candidate.img" >/dev/null )
cmp -s "$stage/check/kernel" "$image"
cmp -s "$stage/check/ramdisk.cpio" "$stage/stock/ramdisk.cpio"
[[ $(stat -c %s "$stage/candidate.img") -eq 67108864 ]]
[[ $(sha256 "$stage/candidate.img") == "$expected_output" ]]
install -m 0644 "$stage/candidate.img" "$output"
echo "BOOT_IMAGE=$output"
echo "BOOT_SHA256=$expected_output"

