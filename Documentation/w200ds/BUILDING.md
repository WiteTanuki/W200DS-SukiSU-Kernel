# W200DS 发布构建说明 / Building the W200DS release

[简体中文](#简体中文) | [English](#english)

## 简体中文

验收发布版本使用 Ubuntu 22.04（WSL2）构建，源码路径只包含 ASCII 字符。内核 Makefile 会拒绝包含空格或冒号的源码路径。

### 固定输入

- Clang：Android clang 12.0.5，构建号 `r416183b`（Android build 7284624）
- 内核配置：`arch/arm64/configs/w200ds_r11_defconfig`
- 配置 SHA-256：`f0a99476257ad85eaf5669a9dc4e082fca4435c8e8c7aea6aca83cb1213ff897`
- KernelPatch 运行时：Android NDK r29（`29.0.14206865`）
- 可复现运行时时间戳：`1787969194`

本仓库不再分发这些工具链。请从各自官方上游获取，并显式设置 `CLANG_BIN` 和 `ANDROID_NDK_HOME`。

### 构建流水线

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

每个辅助程序都会拒绝不符合预期的固定输入，并核对验收输出的 SHA-256。该流程有意只支持一种源码、配置和工具链组合；任何输入发生变化时，都应通过新的审查与测试周期更新发布清单，而不是削弱检查来让构建通过。

### 重新打包 boot

本仓库不分发 OEM 固件或 Magisk。请提供你自己的、与设备匹配的原厂 boot，以及已测试的同一 `magiskboot` 可执行文件：

```sh
./pack-w200ds-boot.sh STOCK_BOOT Image.k11f0-marked MAGISKBOOT OUTPUT_BOOT
```

该辅助程序只替换内核；重解包后原厂 ramdisk 必须逐字节保持一致；同时检查 64 MiB 容器大小和验收输出哈希。它不会连接或写入设备。

### 验收输出标识

- 原始 Image：`ec46347cfc1ad9b2ce765d010afd0bf88ab742e888ec4568507aaf39a4cfafb0`
- 后链接 Image：`63d853aa9ee85e339ccd2c4c7fac2435888eb45b0c5f6d141477bec8bb0caae3`
- 已标记 Image：`244e6dc3efab05ecbfc3de6ded31714a0cb87e2a7c7e02bcfaff1f1603f17e06`
- Boot 镜像：`a0a52b34a1de45ae4dedf8298da65f87bb423e8f0eb78a7022ee427524210055`

---

## English

The accepted release was built in Ubuntu 22.04 under WSL2 from an ASCII-only
path. The kernel Makefile rejects source paths containing spaces or colons.

### Fixed inputs

- Clang: Android clang 12.0.5, build `r416183b` (Android build 7284624)
- Kernel config: `arch/arm64/configs/w200ds_r11_defconfig`
- Config SHA-256: `f0a99476257ad85eaf5669a9dc4e082fca4435c8e8c7aea6aca83cb1213ff897`
- KernelPatch runtime: Android NDK r29 (`29.0.14206865`)
- Reproducible runtime epoch: `1787969194`

The toolchains themselves are not redistributed here. Obtain them from their
official upstreams and set `CLANG_BIN` and `ANDROID_NDK_HOME` explicitly.

### Pipeline

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

### Repacking boot

The repository does not distribute OEM firmware or Magisk. Supply your own
matching stock boot and the exact tested `magiskboot` executable:

```sh
./pack-w200ds-boot.sh STOCK_BOOT Image.k11f0-marked MAGISKBOOT OUTPUT_BOOT
```

The helper replaces only the kernel, requires the stock ramdisk to remain
byte-identical after re-unpack, checks the 64 MiB container size and verifies
the accepted output hash. It never connects to or writes a device.

### Accepted output identities

- Raw Image: `ec46347cfc1ad9b2ce765d010afd0bf88ab742e888ec4568507aaf39a4cfafb0`
- Post-linked Image: `63d853aa9ee85e339ccd2c4c7fac2435888eb45b0c5f6d141477bec8bb0caae3`
- Marked Image: `244e6dc3efab05ecbfc3de6ded31714a0cb87e2a7c7e02bcfaff1f1603f17e06`
- Boot image: `a0a52b34a1de45ae4dedf8298da65f87bb423e8f0eb78a7022ee427524210055`
