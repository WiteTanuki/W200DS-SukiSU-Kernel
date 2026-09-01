# W200DS SukiSU Kernel

[简体中文](#简体中文) | [English](#english)

Unofficial, device-specific Android 13 kernel for the ZTE W200DS / P720P01.
It combines the tested vendor Linux 5.4.276 baseline with SukiSU (KernelSU),
SUSFS inline hooks and KernelPatch runtime support.

> [!WARNING]
> This release is only for the exact firmware fingerprint listed below. An
> unlocked bootloader and a known-good copy of your own stock `boot_a` are
> required. A wrong image can leave the device unable to boot. No warranty is
> provided.

This project is not affiliated with or endorsed by ZTE, KernelSU, SukiSU,
SUSFS, KernelPatch or Magisk.

---

## 简体中文

### 项目定位

本仓库以 W200DS 原厂 Linux 5.4.276 内核为基线，集成 SukiSU / KernelSU、
SUSFS 2.2.0 inline hook、SukiSU 模块框架和 KernelPatch runtime KPM。它是
**W200DS 专用内核源码与可复现构建工程**，不是 ROM、通用 GKI、一键 Root 工具、
固件包、救砖包或第三方模块合集。

Manager APK、Magisk/KernelSU 模块、OEM 固件、分区备份、FDL loader、密钥和私有
测试日志均不在本仓库中。

> [!CAUTION]
> 只有下表中的设备、固件、活动槽位和 `boot_a` 大小全部一致时才可使用。
> 错误镜像可能导致设备无法启动。开始前必须准备自己的已验证原厂 `boot_a`、
> 解锁 bootloader，并先阅读安装与恢复文档。

### 唯一受测基线

| 项目 | 受测值 |
|---|---|
| 设备 / 产品 | ZTE W200DS / P720P01 |
| 系统指纹 | `ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys` |
| Android / 架构 | Android 13 / arm64 |
| 内核 | `5.4.276-android12-9-g83f519fbd509` |
| 活动槽位 | A (`_a`) |
| `boot_a` 大小 | `0x4000000`（64 MiB） |
| SELinux | Enforcing |
| SukiSU Manager / 驱动代号 | 40856 |
| SUSFS | 2.2.0，inline hook |
| 初版标记 | `k11f0@` |
| 初版源码标签 | [`v0.1.0-rc1`](https://github.com/WiteTanuki/W200DS-SukiSU-Kernel/tree/v0.1.0-rc1) |

任一项目不同都应视为 **不兼容且未经测试**。名称相似、同为 Android 13 或分区
大小相同，都不能代替完整指纹核对。

### 初版功能矩阵

| 能力 | `v0.1.0-rc1` | 说明 |
|---|---:|---|
| SukiSU / KernelSU Root | ✅ 已验证 | Manager 能识别内核并管理授权 |
| SukiSU 模块 | ✅ 已验证 | 受测模块栈可在正常重启后加载 |
| SUSFS 2.2.0 | ✅ 已验证 | 实际隐藏效果取决于模块和配置 |
| SELinux Enforcing | ✅ 已验证 | 不以 Permissive 作为运行前提 |
| runtime KPM | ✅ 已验证 | 已验证加载/卸载路径；仅供高级显式使用 |
| embedded KPM | ➖ 未包含 | 发布镜像内嵌 KPM 数量为 0 |
| 隐藏 SELinux 修改 | ❌ 未包含 | 后续研究版本不属于此标签 |
| KernelSU ADB Root | ❌ 未启用 | ADB 默认仍为普通 Shell |

初版候选已在唯一受测设备上完成启动、Manager 识别、SELinux Enforcing、模块
重启持久性和回退验证。应用、模块、服务端策略或固件更新后结果可能变化。

### 获取源码与发布物

```sh
git clone https://github.com/WiteTanuki/W200DS-SukiSU-Kernel.git
cd W200DS-SukiSU-Kernel
git checkout v0.1.0-rc1
```

- 源码以 Git 标签为准；`main` 可能包含标签之后的文档修订。
- 预编译 boot 如有发布，只会放在
  [GitHub Releases](https://github.com/WiteTanuki/W200DS-SukiSU-Kernel/releases)，
  不会提交到 Git 历史。
- 不要使用聊天群、网盘或论坛中的同名镜像，除非 SHA-256 与本仓库记录完全一致。
- SukiSU Manager 和模块请从各自官方项目获取，本仓库不捆绑 APK 或模块。

初版受测 boot 身份：

```text
SHA-256  a0a52b34a1de45ae4dedf8298da65f87bb423e8f0eb78a7022ee427524210055
Size     67,108,864 bytes
Marker   k11f0@
```

Linux / WSL：

```sh
sha256sum w200ds-sukisu-v0.1.0-rc1-boot.img
```

PowerShell 7：

```powershell
(Get-FileHash .\w200ds-sukisu-v0.1.0-rc1-boot.img -Algorithm SHA256).Hash.ToLowerInvariant()
```

没有发布资产、哈希不一致或文件不是 64 MiB 时，不要刷入。

### 快速构建

受测环境为 Ubuntu 22.04 / WSL2。源码路径必须只含 ASCII，且不能包含空格或冒号。

固定输入：

- Android Clang 12.0.5 `r416183b`（Android build 7284624）；
- `arch/arm64/configs/w200ds_r11_defconfig`；
- Android NDK r29 `29.0.14206865`（构建 KernelPatch runtime 时需要）；
- Full LTO，arm64；
- 与目标固件完全匹配的 stock boot 和受测 `magiskboot`（不随仓库分发）。

最小内核构建：

```sh
export CLANG_BIN=/absolute/path/to/clang-r416183b/bin
OUT="$PWD/out/w200ds-r11" JOBS=8 ./build-w200ds.sh
```

完整发布管线：

```text
source + defconfig
  -> Full LTO Image + modules
  -> fixed KernelPatch runtime post-link
  -> unique release marker
  -> replace kernel in matching stock boot
  -> re-unpack, size and SHA-256 acceptance
```

```sh
export ANDROID_NDK_HOME=/absolute/path/to/android-ndk-r29
./build-kernelpatch-runtime.sh "$PWD/out/kernelpatch-runtime"
./postlink-w200ds.sh \
  "$PWD/out/w200ds-r11/arch/arm64/boot/Image" \
  "$PWD/out/kernelpatch-runtime" \
  "$PWD/out/Image.kpm-postlinked"
./mark-release-image.py \
  "$PWD/out/Image.kpm-postlinked" \
  "$PWD/out/Image.k11f0-marked"
./pack-w200ds-boot.sh STOCK_BOOT "$PWD/out/Image.k11f0-marked" MAGISKBOOT OUTPUT_BOOT
```

脚本会拒绝错误 config、工具链或输出哈希。源码、配置或工具链变化后，应建立新
版本并重新审查预期身份，不能为了“让脚本通过”而移除保护检查。完整要求见
[BUILDING.md](Documentation/w200ds/BUILDING.md)。

### 安装前检查与刷入

先阅读 [INSTALL.md](Documentation/w200ds/INSTALL.md) 和
[RECOVERY.md](Documentation/w200ds/RECOVERY.md)。安装前必须：

1. 在设备外保存并验证该固件自己的 stock `boot_a`；
2. 解锁 bootloader，保证电量与 USB 连接稳定；
3. 确认 ADB 中的系统指纹与上表完全相同；
4. 在真 bootloader 中确认 `current-slot=a`、`is-userspace=no`；
5. 确认 `boot_a=0x4000000`；
6. 只连接一个目标设备，并核对镜像大小与 SHA-256。

```text
adb shell getprop ro.build.fingerprint
adb reboot bootloader
fastboot getvar current-slot
fastboot getvar is-userspace
fastboot getvar partition-size:boot_a
fastboot flash boot_a w200ds-sukisu-v0.1.0-rc1-boot.img
fastboot reboot
```

只写入 `boot_a` 一次。不要使用 `fastboot boot`，不要切换槽位、写其他分区或自动
重试。Android 启动后，先确认 `k11f0@`、slot A、SELinux Enforcing 和 Manager
显示“工作中”，再逐个启用模块。

### 回退原则

如果 Android 未正常启动：

1. 停止重复开机和重复刷写；
2. 优先使用正常 bootloader / recovery 路径；
3. 只把该固件已验证的 stock/control boot 恢复到 `boot_a`；
4. 不要 erase、format、`set_active`，也不要尝试其他设备镜像；
5. 无法进入 bootloader 时，停止并寻求已独立验证的 W200DS 专用恢复帮助。

本仓库不提供 FDL loader、OEM 固件、分区镜像或无人值守救砖脚本。

### 重要限制

- 仅支持上表中的 W200DS / P720P01 固件；没有 B 槽或其他固件的测试结论。
- Root 隐藏是“尽力而为”，不能保证通过所有本地检测或未来远程完整性策略。
- 初版没有 SELinux-hide、KernelSU ADB Root 或 embedded KPM。
- runtime KPM 是高级接口，普通 Root/隐藏使用不需要 KPM；仓库不提供 KPM payload。
- OEM `/system/bin/su` 与 `/system/bin/sh` 的别名/共享 inode 行为不能仅靠替换
  内核安全解决；隐藏该 inode 也可能误伤 Shell。
- 模块兼容性取决于模块版本、配置和依赖；第三方模块不属于本项目支持范围。
- boot ramdisk 来自用户自己的匹配 OEM 镜像，本仓库不分发 OEM ramdisk。

详见 [KNOWN-LIMITATIONS.md](Documentation/w200ds/KNOWN-LIMITATIONS.md)。

### 文档导航

| 文档 | 内容 |
|---|---|
| [BUILDING.md](Documentation/w200ds/BUILDING.md) | 固定工具链、构建与输出身份 |
| [INSTALL.md](Documentation/w200ds/INSTALL.md) | 安装前置条件和最小刷入流程 |
| [RECOVERY.md](Documentation/w200ds/RECOVERY.md) | 回退准备与停止条件 |
| [KNOWN-LIMITATIONS.md](Documentation/w200ds/KNOWN-LIMITATIONS.md) | 初版能力边界 |
| [SOURCE-REVISIONS.md](Documentation/w200ds/SOURCE-REVISIONS.md) | 上游版本、内核与 OEM ABI 身份 |
| [THIRD-PARTY-NOTICES.md](Documentation/w200ds/THIRD-PARTY-NOTICES.md) | 第三方来源、许可和排除项 |
| [CONTRIBUTING.md](CONTRIBUTING.md) | 提交补丁的最小规范 |

以上 W200DS 专用文档均提供完整简体中文，并保留英文对照。上游 Linux 原始文档
保持原文，避免改变上游技术语义、版权或许可记录。

### 问题反馈与贡献

Issue 应只包含复现问题所必需的信息：版本/标签、完整固件指纹、`uname -a`、
Manager 版本、仅与问题相关的模块及其版本、最小复现步骤、预期/实际结果、经过
裁剪和脱敏的日志，以及是否仍有 stock boot 回退路径。请先确认问题在全部模块
关闭后是否仍可复现。

请删除序列号、账户、token、密钥、keybox、attestation 数据及个人信息；不要上传
OEM 固件、分区 dump、私有模块、个人应用清单、无关截图或完整设备日志。目标应用
身份与问题无关时，请使用“目标应用”等通用称呼。内核补丁应遵循 Linux kernel
coding style，运行 `scripts/checkpatch.pl`，保留来源和 SPDX 信息，并使用
`git commit -s` 添加真实的 `Signed-off-by`。详见
[CONTRIBUTING.md](CONTRIBUTING.md)。

### 许可证、来源与致谢

Linux 内核树保留 `COPYING`、`LICENSES/` 和逐文件 SPDX 声明。SukiSU / KernelSU、
KernelPatch 与 SUSFS 衍生部分保留各自版权、来源和许可文本；本仓库不重新许可
第三方代码。SUSFS 衍生代码与 GPL-2.0-only 内核组合分发的许可兼容性仍需相关
权利人澄清，重新分发者应先阅读
[THIRD-PARTY-NOTICES.md](Documentation/w200ds/THIRD-PARTY-NOTICES.md) 和
[NOTICE](NOTICE)。

感谢 [Linux kernel](https://www.kernel.org/)、
[Android Common Kernel](https://source.android.com/docs/core/architecture/kernel)、
[KernelSU](https://github.com/tiann/KernelSU)、
[SukiSU Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra)、
[SUSFS](https://gitlab.com/simonpunk/susfs4ksu) 和
[KernelPatch](https://github.com/bmax121/KernelPatch) 的维护者与贡献者。

---

## English

### Overview

This repository starts from the vendor Linux 5.4.276 kernel for the W200DS and
integrates SukiSU / KernelSU, SUSFS 2.2.0 inline hooks, the SukiSU module
framework and KernelPatch runtime KPM. It is a **device-specific kernel source
and reproducible build project**, not a ROM, generic GKI, one-click root tool,
firmware package, rescue bundle or module collection.

Manager APKs, Magisk/KernelSU modules, OEM firmware, partition backups, FDL
loaders, keys and private test logs are deliberately excluded.

> [!CAUTION]
> Use this project only when the device, firmware, active slot and `boot_a` size
> exactly match the table below. A wrong image can leave the device unbootable.
> Prepare your own verified stock `boot_a`, unlock the bootloader, and read the
> installation and recovery guides first.

### Only tested baseline

| Item | Tested value |
|---|---|
| Device / product | ZTE W200DS / P720P01 |
| Firmware fingerprint | `ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys` |
| Android / architecture | Android 13 / arm64 |
| Kernel | `5.4.276-android12-9-g83f519fbd509` |
| Active slot | A (`_a`) |
| `boot_a` size | `0x4000000` (64 MiB) |
| SELinux | Enforcing |
| SukiSU Manager / driver code | 40856 |
| SUSFS | 2.2.0, inline hook |
| Initial marker | `k11f0@` |
| Initial source tag | [`v0.1.0-rc1`](https://github.com/WiteTanuki/W200DS-SukiSU-Kernel/tree/v0.1.0-rc1) |

Treat any difference as **incompatible and untested**. A similar model name,
Android 13, or the same partition size is not a substitute for an exact
fingerprint match.

### Initial feature matrix

| Capability | `v0.1.0-rc1` | Notes |
|---|---:|---|
| SukiSU / KernelSU root | ✅ Tested | Manager detects the kernel and manages grants |
| SukiSU modules | ✅ Tested | The tested module stack loaded after a normal reboot |
| SUSFS 2.2.0 | ✅ Tested | Concealment depends on module and configuration |
| SELinux Enforcing | ✅ Tested | Permissive mode is not a runtime prerequisite |
| Runtime KPM | ✅ Tested | Load/unload path validated; advanced explicit use only |
| Embedded KPM | ➖ Not included | Embedded KPM count is zero |
| Hide SELinux modifications | ❌ Not included | Later research builds are outside this tag |
| KernelSU ADB Root | ❌ Disabled | ADB remains an ordinary shell by default |

The initial candidate passed boot, Manager detection, SELinux Enforcing, module
reboot persistence and rollback checks on the one tested device. Results can change with an app, module, server policy or firmware
update.

### Getting the source and artifacts

```sh
git clone https://github.com/WiteTanuki/W200DS-SukiSU-Kernel.git
cd W200DS-SukiSU-Kernel
git checkout v0.1.0-rc1
```

- Git tags are the source authority; `main` may contain documentation changes
  made after a tag.
- If a prebuilt boot image is published, it will appear only under
  [GitHub Releases](https://github.com/WiteTanuki/W200DS-SukiSU-Kernel/releases),
  never in Git history.
- Do not trust a same-named image from a forum, chat or mirror unless its SHA-256
  exactly matches this repository's record.
- Obtain SukiSU Manager and modules from their own official projects. This
  repository does not bundle APKs or modules.

Accepted initial boot identity:

```text
SHA-256  a0a52b34a1de45ae4dedf8298da65f87bb423e8f0eb78a7022ee427524210055
Size     67,108,864 bytes
Marker   k11f0@
```

Linux / WSL:

```sh
sha256sum w200ds-sukisu-v0.1.0-rc1-boot.img
```

PowerShell 7:

```powershell
(Get-FileHash .\w200ds-sukisu-v0.1.0-rc1-boot.img -Algorithm SHA256).Hash.ToLowerInvariant()
```

Do not flash when no release asset exists, the digest differs, or the image is
not exactly 64 MiB.

### Quick build

The accepted environment was Ubuntu 22.04 under WSL2. Keep the source in an
ASCII-only path without spaces or colons.

Pinned inputs:

- Android Clang 12.0.5 `r416183b` (Android build 7284624);
- `arch/arm64/configs/w200ds_r11_defconfig`;
- Android NDK r29 `29.0.14206865` for the KernelPatch runtime;
- Full LTO for arm64; and
- your own matching stock boot plus the tested `magiskboot` executable.

Minimal kernel build:

```sh
export CLANG_BIN=/absolute/path/to/clang-r416183b/bin
OUT="$PWD/out/w200ds-r11" JOBS=8 ./build-w200ds.sh
```

Full release pipeline:

```text
source + defconfig
  -> Full LTO Image + modules
  -> fixed KernelPatch runtime post-link
  -> unique release marker
  -> replace kernel in matching stock boot
  -> re-unpack, size and SHA-256 acceptance
```

```sh
export ANDROID_NDK_HOME=/absolute/path/to/android-ndk-r29
./build-kernelpatch-runtime.sh "$PWD/out/kernelpatch-runtime"
./postlink-w200ds.sh \
  "$PWD/out/w200ds-r11/arch/arm64/boot/Image" \
  "$PWD/out/kernelpatch-runtime" \
  "$PWD/out/Image.kpm-postlinked"
./mark-release-image.py \
  "$PWD/out/Image.kpm-postlinked" \
  "$PWD/out/Image.k11f0-marked"
./pack-w200ds-boot.sh STOCK_BOOT "$PWD/out/Image.k11f0-marked" MAGISKBOOT OUTPUT_BOOT
```

The wrappers reject unexpected configs, toolchains and output digests. When
source, config or toolchain changes, create a new version and review the new
expected identities; do not remove checks merely to make a build pass. See
[BUILDING.md](Documentation/w200ds/BUILDING.md).

### Pre-install checks and flashing

Read [INSTALL.md](Documentation/w200ds/INSTALL.md) and
[RECOVERY.md](Documentation/w200ds/RECOVERY.md) first. Before installation:

1. keep a verified stock `boot_a` for this firmware outside the device;
2. unlock the bootloader and use a charged device with a stable USB connection;
3. match the ADB firmware fingerprint exactly;
4. confirm `current-slot=a` and `is-userspace=no` in the real bootloader;
5. confirm `boot_a=0x4000000`; and
6. connect one target only and verify the image size and SHA-256.

```text
adb shell getprop ro.build.fingerprint
adb reboot bootloader
fastboot getvar current-slot
fastboot getvar is-userspace
fastboot getvar partition-size:boot_a
fastboot flash boot_a w200ds-sukisu-v0.1.0-rc1-boot.img
fastboot reboot
```

Write `boot_a` once. Do not use `fastboot boot`, switch slots, touch another
partition or automate retries. After Android starts, confirm `k11f0@`, slot A,
SELinux Enforcing and a working SukiSU Manager before enabling modules one by
one.

### Recovery principles

If Android does not boot normally:

1. stop repeated boots and repeated writes;
2. prefer the normal bootloader / recovery path;
3. restore only the verified stock/control boot for that firmware to `boot_a`;
4. never erase, format, run `set_active`, or try another device's image; and
5. if the bootloader is unreachable, stop and seek independently validated,
   W200DS-specific recovery help.

This repository intentionally ships no FDL loader, OEM firmware, partition image
or unattended rescue script.

### Important limitations

- Only the listed W200DS / P720P01 firmware is supported. There is no B-slot or
  other-firmware test claim.
- Root concealment is best-effort and cannot guarantee every local check or
  future remote integrity policy.
- The initial release has no SELinux-hide, KernelSU ADB Root or embedded KPM.
- Runtime KPM is an advanced interface and is unnecessary for ordinary root or
  concealment use; no KPM payload is provided.
- The OEM `/system/bin/su` and `/system/bin/sh` alias/shared-inode behavior cannot
  be safely solved merely by replacing the kernel; hiding that inode can also
  break the shell.
- Module compatibility depends on module version, configuration and dependencies.
  Third-party modules are outside this project's support scope.
- The boot ramdisk comes from the user's matching OEM image. This repository does
  not distribute an OEM ramdisk.

See [KNOWN-LIMITATIONS.md](Documentation/w200ds/KNOWN-LIMITATIONS.md).

### Documentation map

| Document | Purpose |
|---|---|
| [BUILDING.md](Documentation/w200ds/BUILDING.md) | Pinned toolchain, build and output identities |
| [INSTALL.md](Documentation/w200ds/INSTALL.md) | Preconditions and minimal flashing flow |
| [RECOVERY.md](Documentation/w200ds/RECOVERY.md) | Rollback preparation and stop conditions |
| [KNOWN-LIMITATIONS.md](Documentation/w200ds/KNOWN-LIMITATIONS.md) | Initial capability boundaries |
| [SOURCE-REVISIONS.md](Documentation/w200ds/SOURCE-REVISIONS.md) | Upstream, kernel and OEM ABI identities |
| [THIRD-PARTY-NOTICES.md](Documentation/w200ds/THIRD-PARTY-NOTICES.md) | Provenance, licensing and excluded material |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Minimum patch-submission rules |

All W200DS-specific documents above provide complete Simplified Chinese text and
retain an English counterpart. Original upstream Linux documentation stays in
its source language to preserve upstream technical, copyright and license text.

### Issues and contributions

An issue should include only what is necessary to reproduce it: the release/tag,
complete firmware fingerprint, `uname -a`, Manager version, only relevant
modules and versions, minimal reproduction, expected and actual results, trimmed
and sanitized logs, and whether a stock-boot recovery path is still available.
First confirm whether the issue persists with all modules disabled.

Remove serial numbers, accounts, tokens, keys, keyboxes, attestation data and
other personal information. Never upload OEM firmware, partition dumps, private
modules, personal app inventories, unrelated screenshots or complete device
logs. Use a generic name such as “target app” when its identity is irrelevant.
Kernel patches should follow Linux kernel coding style, run
`scripts/checkpatch.pl`, preserve provenance and SPDX notices, and add a genuine
`Signed-off-by` with `git commit -s`. See [CONTRIBUTING.md](CONTRIBUTING.md).

### Licensing, provenance and acknowledgements

The Linux tree retains `COPYING`, `LICENSES/` and per-file SPDX declarations.
SukiSU / KernelSU, KernelPatch and SUSFS-derived parts retain their respective
copyright, provenance and license texts; this repository does not relicense
third-party work. License compatibility for distributing SUSFS-derived code in a
GPL-2.0-only kernel remains a question for the relevant rightsholders. Read
[THIRD-PARTY-NOTICES.md](Documentation/w200ds/THIRD-PARTY-NOTICES.md) and
[NOTICE](NOTICE) before redistribution.

Thanks to the maintainers and contributors behind the
[Linux kernel](https://www.kernel.org/),
[Android Common Kernel](https://source.android.com/docs/core/architecture/kernel),
[KernelSU](https://github.com/tiann/KernelSU),
[SukiSU Ultra](https://github.com/SukiSU-Ultra/SukiSU-Ultra),
[SUSFS](https://gitlab.com/simonpunk/susfs4ksu), and
[KernelPatch](https://github.com/bmax121/KernelPatch).
