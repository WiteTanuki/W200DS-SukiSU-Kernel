# W200DS SukiSU 移植 Helper / W200DS SukiSU Porting Helper

[简体中文](#简体中文) | [English](#english)

> 本文面向需要复现构建、理解 boot 分层或排查移植问题的使用者。若你只想在已通过恢复包解锁 BL 的受支持 W200DS 上安装发布版，请先阅读 [小白安装手册](PORTING.md)。
>
> This document is for reproducing builds, understanding boot-image layers, and troubleshooting ports. To install the release on a supported W200DS whose bootloader was already unlocked with the recovery package, start with the [beginner installation guide](PORTING.md).

## 简体中文

### 1. 先区分“安装”和“移植”

W200DS `v0.2.0` 的最终设备变更确实只有一项：把验收通过的 64 MiB
`boot.img` 写入活动 A 槽的 `boot_a`。这不等于移植工作只是在现成 boot 上做一次
简单修改。

| 层级 | 实际工作 | 是否改变设备分区 |
|---|---|---:|
| 安装发布版 | 核对精确固件、bootloader、槽位、大小和哈希后写入 `boot_a` | 是，仅 `boot_a` |
| 复现本项目 | 构建 W200DS 兼容内核，集成 SukiSU/SUSFS/KPM，再装入 OEM boot 容器 | 否，主机侧工作 |
| 移植到其他固件/设备 | 重新解决源码、配置、ABI、签名、启动镜像和实机验收 | 需要新的完整周期 |

因此可以把本项目概括为：**源码级移植，boot 级交付**。

### 2. 恢复包在本项目中的真实作用

开发前设备使用的私有恢复包包含完整分区备份、FDL loader、`uboot`、`vbmeta`、
`super`、`boot_a` 等 OEM 资产，其批处理
会通过 UNISOC FDL 通道执行全分区写入。该恢复路径不属于 SukiSU 内核的安装流程，
也不能随本仓库公开分发。

与本项目直接相关的输入只有其中的 `boot_a.bin`：

```text
Size      67,108,864 bytes (64 MiB)
SHA-256   9ddb0f2b4bd2ce86cacad2d7091deb02edc87a564937e68560172b1f7877350e
Header    Android boot image header v4
Kernel    raw, 39,535,104 bytes
Ramdisk   lz4_legacy, 1,488,570 bytes
OS        Android 13 / SPL 2025-02
```

使用受测 `magiskboot` 解包后，`cpio ramdisk.cpio test` 返回 `0`；按 Magisk 官方定义，
这表示 ramdisk 没有 Magisk 标志。恢复包目录中也没有 Magisk APK 或 Magisk patched
boot。本项目另有一个历史 Magisk 回退 boot，SHA-256 为
`fab1824f8255900a27412d726665da944397304f687aaf636a5d5cdf180fb203`，它与恢复包
`boot_a.bin` 不是同一文件。

所以，现有证据支持以下边界：

- 恢复包为本项目提供了匹配目标固件的 OEM boot 基底和灾难恢复来源；
- 设备的 bootloader 解锁状态与 Magisk 使用历史是独立前置状态；
- 不能仅凭该包断言 Magisk 是由哪个文件或哪一步安装的；
- SukiSU 发布 boot 没有继续使用 Magisk ramdisk root，两种 root 方案不要叠加安装。

### 3. W200DS 的启动镜像分层

该设备是 Android 13、Linux 5.4、A/B/Virtual A/B 设备，存在 `boot`、`init_boot`、
`vendor_boot`、`dtbo` 和 `vbmeta` 等独立分区。AOSP 对 boot header v4 的定义说明，
`boot` 至少承载 kernel，并可承载 generic ramdisk；`vendor_boot` 承载设备相关 ramdisk、
DTB 和 bootconfig，Android 13 设备还可能用 `init_boot` 承载 generic ramdisk。

W200DS 的受测 OEM `boot_a` 实物同时包含 kernel 与 ramdisk。本项目选择：

```text
OEM boot_a
  ├─ header / OS version / page size       保持
  ├─ OEM ramdisk                            逐字节保持
  ├─ OEM AVB/vbmeta 尾部和容器布局           保持并由同一工具重建
  └─ kernel                                 替换为 W200DS SukiSU Image
```

`init_boot`、`vendor_boot`、`dtbo`、`vbmeta`、`super` 和 bootloader 均不参与 v0.2.0
正常安装。bootloader 必须事先处于真实解锁状态；解锁不是 SukiSU boot 的功能。

### 4. 实际移植流程

#### 阶段 A：冻结设备和固件身份

只接受以下已测试基线：

```text
Device       ZTE W200DS / P720P01
Fingerprint  ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys
Android      13 / arm64
Kernel       5.4.276-android12-9-g83f519fbd509
Slot         A
boot_a       0x4000000
```

先在设备外保存匹配固件的 `boot_a`，记录文件大小和 SHA-256。名称相同、同为 Android
13、同为 UMS9620 或分区同为 64 MiB 都不足以证明兼容。

#### 阶段 B：建立可启动的源码基线

W200DS 没有直接可用的一键通用内核。项目从同平台的 ZTE Linux 5.4.276 vendor-derived
源码建立基线，再恢复 W200DS 所需配置、版本后缀和 vendor 源文件。第一目标不是加入
root，而是重建：

- 精确 kernel release `5.4.276-android12-9-g83f519fbd509`；
- W200DS 需要的 built-in 驱动与模块配置；
- Full LTO、CFI、`CONFIG_MODVERSIONS` 和强制模块签名；
- OEM 模块公共证书和 vendor 模块 ABI。

最终 v0.2.0 固定为 170 个重建模块、205 个 OEM 模块、10,324/10,324 个导入 CRC
闭合，`module_layout=0x2f279e7b`。如果只拿一个同版本通用 Image 塞进 boot，常见结果
会是 vendor 模块无法加载、硬件失效或早期启动失败。

#### 阶段 C：源码级集成 SukiSU、SUSFS 与 KPM

W200DS 的 5.4 内核采用 built-in/manual integration 路线。固定组件版本见
[`SOURCE-REVISIONS.md`](SOURCE-REVISIONS.md)，最终配置见
`arch/arm64/configs/w200ds_r11_defconfig`。核心工作包括：

- 在内核源码中集成固定 SukiSU/KernelSU driver，而不是向 ramdisk 放入 `su`；
- 按 5.4 VFS、procfs、内存管理和 SELinux 接口移植 SUSFS 2.2.0 能力；
- 适配 SukiSU Manager 40856 的 Profile/UAPI；
- 保留 SELinux Enforcing、Full LTO、CFI、MODVERSIONS 和模块签名；
- 构建 KernelPatch runtime，使 KPM 可用，但发布镜像内嵌 KPM 数量保持为 0；
- 对 OEM 模块 signer、vermagic、导入符号和 CRC 做完整闭合检查。

上游 SukiSU 也明确说明：non-GKI OEM 内核高度碎片化，不能由项目提供通用 boot；必须
先有可启动的设备内核源码，再针对该树进行手工集成。

#### 阶段 D：可复现构建裸内核

检出精确发布标签：

```sh
git clone https://github.com/WiteTanuki/W200DS-SukiSU-Kernel.git
cd W200DS-SukiSU-Kernel
git checkout v0.2.0
```

使用 Ubuntu 22.04 / WSL2、Android Clang `r416183b` 和固定 defconfig：

```sh
export CLANG_BIN=/absolute/path/to/clang-r416183b/bin
OUT="$PWD/out/w200ds-r11" JOBS=8 ./build-w200ds.sh
```

脚本固定构建用户、主机、时间戳和路径映射，并要求原始 Image 为：

```text
SHA-256  be5f744963f9eca5b7b84e37747cb191aabc28d4019d403e31f43f221edaf16e
```

#### 阶段 E：KernelPatch 后链接与发布标记

```sh
export ANDROID_NDK_HOME=/absolute/path/to/android-ndk-r29
./build-kernelpatch-runtime.sh "$PWD/out/kernelpatch-runtime"
./postlink-w200ds.sh \
  "$PWD/out/w200ds-r11/arch/arm64/boot/Image" \
  "$PWD/out/kernelpatch-runtime" \
  "$PWD/out/Image.kpm-postlinked"
./mark-release-image.py \
  "$PWD/out/Image.kpm-postlinked" \
  "$PWD/out/w200ds-sukisu-v0.2.0-Image"
```

`postlink-w200ds.sh` 写入匹配的 KernelPatch runtime，并验证 `patched=true`、`num=0`。
标记脚本只对两处等长构建标记做受控替换。最终裸 Image 固定为：

```text
Size      39,854,288 bytes
SHA-256   19532e425e2893cdd983df069db48ec90699ace4c065cf745af3a56ca3ca90b7
Marker    v0.2ga1@w200ds
```

#### 阶段 F：把新 kernel 装入 OEM boot

这里才进入“boot 文件修改”阶段：

```sh
./pack-w200ds-boot.sh \
  STOCK_BOOT \
  "$PWD/out/w200ds-sukisu-v0.2.0-Image" \
  MAGISKBOOT \
  "$PWD/out/w200ds-sukisu-v0.2.0-boot.img"
```

`magiskboot` 在这里仅作为 boot 镜像解包/重打包工具使用，不会把 Magisk root 安装进
ramdisk。脚本要求：

- `STOCK_BOOT` 恰为上述 OEM boot 哈希和 64 MiB 大小；
- 内嵌 kernel 恰为发布 Image；
- 重解包后的 ramdisk 与 OEM ramdisk 逐字节相同；
- 容器仍为 64 MiB；
- 输出哈希恰为
  `a4e2fc24488ce806ce5282ee438ca1d8761c635edb1d02a20e7fa4cd98320e3e`。

现场重解包数据进一步证明，OEM 与 GA 的 ramdisk SHA-256 都是：

```text
d6c8c4f242f9654e801398e0cd5b05cc366ffd99193403ba71bfd6095d788706
```

两者的差异位于 kernel，而不在 ramdisk。

#### 阶段 G：主机验收与设备采用

主机侧至少检查：

1. config、工具链、源码标签和输出哈希；
2. kernel release、OEM 模块 signer/vermagic/CRC；
3. boot header、ramdisk、容器大小与内嵌 kernel；
4. KernelPatch 状态与发布标记；
5. 两次隔离构建/打包是否逐字节一致。

设备侧仅在已有可靠回退路径时进行一次受控采用：核对唯一设备、真实 bootloader、
A 槽、非 fastbootd 和 `boot_a=0x4000000`，然后只写 `boot_a`。启动后检查 marker、
slot、normal boot、`boot_completed=1`、SELinux Enforcing、SukiSU Manager、模块框架与基础硬件。第三方模块和个人配置不属于发布采用门。
具体安装命令和停止条件见 [`INSTALL.md`](INSTALL.md) 与 [`RECOVERY.md`](RECOVERY.md)。

### 5. 从 Magisk 迁移时需要理解的差异

Magisk 的典型安装会修补 boot/init_boot/recovery 的 ramdisk，并通过 `magiskinit` 接管
早期 init。SukiSU/KernelSU built-in 路线把 root driver 集成到内核源码。本项目因此
没有把历史 Magisk patched boot 当作打包底稿，而是从未被 Magisk 修改的 OEM boot
开始，只替换 kernel。

迁移顺序应当是：

1. 保存 OEM stock boot 和已知可启动回退 boot；
2. 由使用者自行备份其私人模块与配置；
3. 使用 clean OEM ramdisk 打包 SukiSU kernel；
4. 首次启动时保持第三方模块关闭；
5. 确认 Manager 和基础硬件后逐个迁移模块。

Magisk 模块与 KernelSU/SukiSU 模块的安装环境、挂载实现及 Zygisk 依赖并不天然等价，
不能把旧模块目录直接视为兼容证明。

### 6. 移植到另一固件或另一台相似设备

`pack-w200ds-boot.sh` 的固定哈希是发布验收合同，不是通用打包器。另一固件即使仍叫
W200DS，也不能把脚本中的 expected hash 删除后直接刷入。新的目标至少需要重新完成：

- 完整 fingerprint、SPL、boot header、分区大小和 AVB 链识别；
- 对应源码/config/toolchain 的可启动基线；
- vendor 模块 signer、vermagic、符号和 CRC 闭合；
- 新 stock boot 的 ramdisk/header/vbmeta 保真打包；
- 主机可复现性、单变量设备采用、重启/冷启动、基础硬件与模块框架验收；
- 新版本号、独立产物哈希和恢复方案。

如果缺少匹配源码或 OEM 模块 ABI 不能闭合，正确结论是“当前不能安全移植”，而不是
让设备承担试错。

### 7. 项目与用户环境的边界

[KernelSU 安装指南](https://kernelsu.org/guide/installation.html)把 stock boot 备份、设备兼容性和回退放在安装前；[AnyKernel3](https://github.com/osm0sis/AnyKernel3)提供设备、Android 版本和安全补丁门。成熟项目普遍把这些可验证条件作为内核发布合同，把模块与应用配置留给使用者。本项目沿用这一边界：源码移植只验证 SukiSU 官方驱动、Manager 和模块框架；第三方模块组合、账号、应用、keybox、认证数据和私人日志不进入发布基线，也不应出现在默认问题报告中。

### 8. 一手资料

- [AOSP: Boot image header](https://source.android.com/docs/core/architecture/bootloader/boot-image-header)
- [AOSP: Generic boot partition](https://source.android.com/docs/core/architecture/partitions/generic-boot)
- [AOSP: Vendor boot partitions](https://source.android.com/docs/core/architecture/partitions/vendor-boot-partitions)
- [AOSP: Android Verified Boot](https://source.android.com/docs/security/features/verifiedboot/avb)
- [AOSP: Build kernels](https://source.android.com/docs/setup/build/building-kernels)
- [SukiSU: integration guide](https://github.com/SukiSU-Ultra/SukiSU-Ultra/blob/main/docs/guide/how-to-integrate.md)
- [KernelSU: installation](https://kernelsu.org/guide/installation.html)
- [KernelSU: module guide](https://kernelsu.org/guide/module.html)
- [KernelSU: metamodule](https://kernelsu.org/guide/metamodule.html)
- [KernelSU: non-GKI integration](https://kernelsu.org/guide/how-to-integrate-for-non-gki.html)
- [AnyKernel3](https://github.com/osm0sis/AnyKernel3)
- [Zygisk Next](https://github.com/Dr-TSNG/ZygiskNext)
- [LSPosed](https://github.com/LSPosed/LSPosed)
- [LiteGApps](https://github.com/litegapps/litegapps)
- [Magisk: installation](https://topjohnwu.github.io/Magisk/install.html)
- [Magisk: magiskboot tools](https://topjohnwu.github.io/Magisk/tools.html)

---

## English

### 1. Installation and porting are different scopes

The final device-side change in W200DS `v0.2.0` is one 64 MiB image written to
the active `boot_a` partition. The engineering port is much larger than that
single write.

| Scope | Work performed | Device partitions changed |
|---|---|---:|
| Install this release | Match firmware, bootloader, slot, size and digest, then flash `boot_a` | `boot_a` only |
| Reproduce this project | Build a W200DS-compatible kernel, integrate SukiSU/SUSFS/KPM and place it in the OEM boot container | None during host work |
| Port to another firmware/device | Requalify source, config, ABI, signatures, boot images and hardware | A new full cycle |

The project is therefore a **source-level port delivered as a boot image**.

### 2. What the recovery package contributed

The original private recovery package contains full partition backups, UNISOC
FDL loaders, bootloader, vbmeta, super and other proprietary assets. Its batch
file performs a full-partition FDL restore. That path is neither the SukiSU
installation method nor redistributable project material.

The one direct build input was its `boot_a.bin`:

```text
Size      67,108,864 bytes (64 MiB)
SHA-256   9ddb0f2b4bd2ce86cacad2d7091deb02edc87a564937e68560172b1f7877350e
Header    Android boot image header v4
Kernel    raw, 39,535,104 bytes
Ramdisk   lz4_legacy, 1,488,570 bytes
OS        Android 13 / SPL 2025-02
```

The tested `magiskboot cpio ramdisk.cpio test` returned `0`, meaning no Magisk
marker according to Magisk's documented bit mask. The package also contains no
Magisk APK or Magisk-patched boot. A separate historical Magisk rollback image
has SHA-256
`fab1824f8255900a27412d726665da944397304f687aaf636a5d5cdf180fb203`;
it is not the recovery package's `boot_a.bin`.

Consequently, the package supplied a firmware-matched OEM boot base and recovery
source. Bootloader unlock and the previous Magisk installation are separate
prerequisites/history. The available package alone does not prove which file or
step installed Magisk, and the SukiSU release does not stack Magisk ramdisk root.

### 3. Boot-chain layers on W200DS

This Android 13, Linux 5.4, A/B device has separate `boot`, `init_boot`,
`vendor_boot`, `dtbo` and `vbmeta` partitions. AOSP documents header-v4 boot,
generic ramdisk placement, vendor ramdisk/DTB placement and AVB as distinct
layers.

The tested W200DS OEM `boot_a` contains both a kernel and a ramdisk. This project
preserves its header, OS fields, OEM ramdisk and container/AVB layout, replacing
only the kernel with the W200DS SukiSU Image. Normal v0.2.0 installation does not
change `init_boot`, `vendor_boot`, `dtbo`, `vbmeta`, `super` or the bootloader.
The bootloader must already be genuinely unlocked.

### 4. The actual porting pipeline

1. **Freeze the target identity.** Match W200DS/P720P01, the exact firmware
   fingerprint, Android 13, arm64, kernel release, slot A and 64 MiB `boot_a`.
2. **Establish a bootable source baseline.** Start from matching ZTE Linux 5.4.276
   vendor-derived source, restore the W200DS config, vendor sources and exact
   release suffix, and keep Full LTO, CFI, MODVERSIONS and forced module signing.
3. **Close the OEM ABI.** v0.2.0 qualifies 170 rebuilt modules, 205 OEM modules,
   10,324/10,324 imported CRCs and `module_layout=0x2f279e7b`.
4. **Integrate SukiSU/SUSFS/KPM in source.** Use the pinned built-in/manual path,
   adapt the Linux 5.4 VFS/proc/mm/SELinux interfaces and Manager UAPI, and retain
   SELinux Enforcing and the OEM module trust chain.
5. **Build reproducibly.** Use Ubuntu 22.04, Clang `r416183b`, the pinned defconfig,
   build identity and epoch through `build-w200ds.sh`.
6. **Post-link KernelPatch and mark the release.** Use the fixed NDK/runtime,
   require `patched=true` and `num=0`, then apply the equal-length release marker.
7. **Repack the OEM boot.** `pack-w200ds-boot.sh` uses `magiskboot` only as an
   unpack/repack utility, replaces `kernel`, requires a byte-identical ramdisk and
   verifies the final 64 MiB output digest.
8. **Qualify before flashing.** Verify config, toolchain, ABI, component hashes,
   boot layout, reproducibility, rollback and one-variable device adoption.

The complete commands are the same ones shown in [`BUILDING.md`](BUILDING.md):

```sh
export CLANG_BIN=/absolute/path/to/clang-r416183b/bin
OUT="$PWD/out/w200ds-r11" JOBS=8 ./build-w200ds.sh

export ANDROID_NDK_HOME=/absolute/path/to/android-ndk-r29
./build-kernelpatch-runtime.sh "$PWD/out/kernelpatch-runtime"
./postlink-w200ds.sh \
  "$PWD/out/w200ds-r11/arch/arm64/boot/Image" \
  "$PWD/out/kernelpatch-runtime" \
  "$PWD/out/Image.kpm-postlinked"
./mark-release-image.py \
  "$PWD/out/Image.kpm-postlinked" \
  "$PWD/out/w200ds-sukisu-v0.2.0-Image"
./pack-w200ds-boot.sh STOCK_BOOT \
  "$PWD/out/w200ds-sukisu-v0.2.0-Image" MAGISKBOOT OUTPUT_BOOT
```

The raw, marked and packaged identities are recorded in [`BUILDING.md`](BUILDING.md)
and the GitHub Release. Re-unpacking locally showed the same ramdisk SHA-256 in
the OEM and GA images:

```text
d6c8c4f242f9654e801398e0cd5b05cc366ffd99193403ba71bfd6095d788706
```

### 5. Migrating from Magisk

Magisk normally patches the boot/init_boot/recovery ramdisk and inserts
`magiskinit`. Built-in SukiSU/KernelSU puts the root driver in kernel source.
This project therefore starts from a clean OEM ramdisk, preserves it and replaces
the kernel. Preserve stock and known-good rollback boots, back up required module
data, boot first with third-party modules disabled, then migrate modules one at a
time. Magisk and KernelSU/SukiSU module environments and Zygisk dependencies are
not automatically equivalent.

### 6. Porting to another firmware or similar device

The fixed hashes in `pack-w200ds-boot.sh` are an acceptance contract, not a
generic repacker. Do not delete them and flash an arbitrary result. A new target
requires a new fingerprint/AVB/layout assessment, bootable source/config baseline,
OEM module signer/vermagic/CRC closure, stock-boot preservation checks,
reproducible host qualification, device/base-hardware and module-framework tests, version and rollback plan. Third-party module stacks and personal configuration stay outside release acceptance.
If matching source or ABI closure is unavailable, the target is not yet safe to
port.

### 7. Project and user-environment boundary

The [KernelSU installation guide](https://kernelsu.org/guide/installation.html) puts stock-boot backup, compatibility, and rollback before installation; [AnyKernel3](https://github.com/osm0sis/AnyKernel3) provides device, Android-version, and security-patch gates. Mature projects treat these verifiable conditions as the kernel-release contract while leaving modules and app configuration to the user. This project follows that boundary: source-port acceptance covers the official SukiSU driver, Manager, and module framework. Third-party stacks, accounts, apps, keyboxes, attestation data, and private logs do not become part of the release baseline or default issue reports.

### 8. Primary references

- [AOSP: Boot image header](https://source.android.com/docs/core/architecture/bootloader/boot-image-header)
- [AOSP: Generic boot partition](https://source.android.com/docs/core/architecture/partitions/generic-boot)
- [AOSP: Vendor boot partitions](https://source.android.com/docs/core/architecture/partitions/vendor-boot-partitions)
- [AOSP: Android Verified Boot](https://source.android.com/docs/security/features/verifiedboot/avb)
- [AOSP: Build kernels](https://source.android.com/docs/setup/build/building-kernels)
- [SukiSU: integration guide](https://github.com/SukiSU-Ultra/SukiSU-Ultra/blob/main/docs/guide/how-to-integrate.md)
- [KernelSU: installation](https://kernelsu.org/guide/installation.html)
- [KernelSU: module guide](https://kernelsu.org/guide/module.html)
- [KernelSU: metamodule](https://kernelsu.org/guide/metamodule.html)
- [KernelSU: non-GKI integration](https://kernelsu.org/guide/how-to-integrate-for-non-gki.html)
- [AnyKernel3](https://github.com/osm0sis/AnyKernel3)
- [Zygisk Next](https://github.com/Dr-TSNG/ZygiskNext)
- [LSPosed](https://github.com/LSPosed/LSPosed)
- [LiteGApps](https://github.com/litegapps/litegapps)
- [Magisk: installation](https://topjohnwu.github.io/Magisk/install.html)
- [Magisk: magiskboot tools](https://topjohnwu.github.io/Magisk/tools.html)
