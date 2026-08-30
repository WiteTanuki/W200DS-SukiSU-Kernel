# Building the W200DS release

The accepted release was built in Ubuntu 22.04 under WSL2 from an ASCII-only
path. The kernel Makefile rejects source paths containing spaces or colons.

## Fixed inputs

- Clang: Android clang 12.0.5, build `r416183b` (Android build 7284624)
- Kernel config: `arch/arm64/configs/w200ds_r11_defconfig`
- Config SHA-256: `f0a99476257ad85eaf5669a9dc4e082fca4435c8e8c7aea6aca83cb1213ff897`
- KernelPatch runtime: Android NDK r29 (`29.0.14206865`)
- Reproducible runtime epoch: `1787969194`

The toolchains themselves are not redistributed here. Obtain them from their
official upstreams and set `CLANG_BIN` and `ANDROID_NDK_HOME` explicitly.

## Pipeline

```sh
export CLANG_BIN=/absolute/path/to/clang-r416183b/bin
OUT=$PWD/out/w200ds-r11 JOBS=8 ./build-w200ds.sh

export ANDROID_NDK_HOME=/absolute/path/to/android-ndk-r29
./build-kernelpatch-runtime.sh "$PWD/out/kernelpatch-runtime"
./postlink-w200ds.sh \
  "$PWD/out/w200ds-r11/arch/arm64/boot/Image" \
  "$PWD/out/kernelpatch-runtime" \
  "$PWD/out/Image.kpm-postlinked"
./mark-release-image.py \
  "$PWD/out/Image.kpm-postlinked" \
  "$PWD/out/Image.k11f0-marked"
```

Each helper refuses unexpected fixed inputs and checks the accepted output
SHA-256. This intentionally targets one source/config/toolchain combination;
when any input changes, update the release manifest through a new review and
test cycle instead of weakening the checks.

## Repacking boot

The repository does not distribute OEM firmware or Magisk. Supply your own
matching stock boot and the exact tested `magiskboot` executable:

```sh
./pack-w200ds-boot.sh STOCK_BOOT Image.k11f0-marked MAGISKBOOT OUTPUT_BOOT
```

The helper replaces only the kernel, requires the stock ramdisk to remain
byte-identical after re-unpack, checks the 64 MiB container size and verifies
the accepted output hash. It never connects to or writes a device.

## Accepted output identities

- Raw Image: `ec46347cfc1ad9b2ce765d010afd0bf88ab742e888ec4568507aaf39a4cfafb0`
- Post-linked Image: `63d853aa9ee85e339ccd2c4c7fac2435888eb45b0c5f6d141477bec8bb0caae3`
- Marked Image: `244e6dc3efab05ecbfc3de6ded31714a0cb87e2a7c7e02bcfaff1f1603f17e06`
- Boot image: `a0a52b34a1de45ae4dedf8298da65f87bb423e8f0eb78a7022ee427524210055`

