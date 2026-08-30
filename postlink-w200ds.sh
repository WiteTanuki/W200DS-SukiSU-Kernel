#!/usr/bin/env bash
set -euo pipefail

input=${1:?usage: $0 INPUT_IMAGE RUNTIME_DIR OUTPUT_IMAGE}
runtime=${2:?usage: $0 INPUT_IMAGE RUNTIME_DIR OUTPUT_IMAGE}
output=${3:?usage: $0 INPUT_IMAGE RUNTIME_DIR OUTPUT_IMAGE}
expected_input=ec46347cfc1ad9b2ce765d010afd0bf88ab742e888ec4568507aaf39a4cfafb0
expected_kpimg=34bab04a68b28fae689d144ad1245c9271cf53a172068340050adf160211cbbb
expected_tools=c6fd32e3235166efc2f5bc784b1486154d16299da550cf4d4a339a960f484cae
expected_output=63d853aa9ee85e339ccd2c4c7fac2435888eb45b0c5f6d141477bec8bb0caae3
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

