#!/usr/bin/env bash
set -euo pipefail

input=${1:?usage: $0 INPUT_IMAGE RUNTIME_DIR OUTPUT_IMAGE}
runtime=${2:?usage: $0 INPUT_IMAGE RUNTIME_DIR OUTPUT_IMAGE}
output=${3:?usage: $0 INPUT_IMAGE RUNTIME_DIR OUTPUT_IMAGE}
expected_input=be5f744963f9eca5b7b84e37747cb191aabc28d4019d403e31f43f221edaf16e
expected_kpimg=34bab04a68b28fae689d144ad1245c9271cf53a172068340050adf160211cbbb
expected_tools=c6fd32e3235166efc2f5bc784b1486154d16299da550cf4d4a339a960f484cae
expected_output=4bf47964827f138c6941b0e0c90f8500763f3b7b9b2ac60e52770c9590152ff7
sha256() { sha256sum "$1" | awk '{print $1}'; }

[[ ! -e "$output" ]]
[[ $(stat -c %s "$input") -eq 39672320 ]]
[[ $(sha256 "$input") == "$expected_input" ]]
[[ $(sha256 "$runtime/kpimg") == "$expected_kpimg" ]]
[[ $(sha256 "$runtime/kptools") == "$expected_tools" ]]
"$runtime/kptools" -l -i "$input" | grep -Fx 'patched=false' >/dev/null
"$runtime/kptools" -p -i "$input" -k "$runtime/kpimg" -o "$output"
[[ $(sha256 "$output") == "$expected_output" ]]
"$runtime/kptools" -l -i "$output" | grep -Fx 'patched=true' >/dev/null
"$runtime/kptools" -l -i "$output" | grep -Fx 'num=0' >/dev/null
echo "POSTLINK_IMAGE=$output"
echo "POSTLINK_SHA256=$(sha256 "$output")"

