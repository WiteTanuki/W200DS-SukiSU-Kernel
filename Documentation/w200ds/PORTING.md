# W200DS SukiSU 小白安装手册 / Beginner Installation Guide

[简体中文](#简体中文) | [English](#english)

> 本手册只适用于已经通过 W200DS 恢复包完成 BL 解锁的设备。它教你安装本项目已经编译并验证的 SukiSU `v0.2.0`，不包含 BL 解锁、内核编译或其他机型移植。
>
> This guide is only for W200DS devices whose bootloader has already been unlocked with the recovery package. It installs the tested SukiSU `v0.2.0` release; it does not cover bootloader unlocking, kernel compilation, or other devices.

## 简体中文

### 1. 先确认你的设备能用

必须同时满足以下条件：

- 设备是 ZTE W200DS / P720P01；
- 系统指纹完全等于：
  `ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys`；
- Android 13，arm64，当前使用 A 槽；
- 已通过原恢复包完成 BL 解锁；
- 电脑外保存着该固件可用的原厂 `boot_a.bin`；
- 设备能正常进入 Android，并已打开 USB 调试。

任何一项不同都不要继续。其他固件、其他槽位和其他型号没有经过验证。

### 2. 准备文件和软件

在 Windows 电脑准备：

1. 一根稳定的数据线，设备电量充足；
2. [Android Platform Tools](https://developer.android.com/tools/releases/platform-tools)，解压到一个简单目录，例如 `C:\platform-tools`；
3. 从本项目 [v0.2.0 Release](https://github.com/WiteTanuki/W200DS-SukiSU-Kernel/releases/tag/v0.2.0) 下载：
   - `w200ds-sukisu-v0.2.0-boot.img`
   - `SHA256SUMS`
4. 从 SukiSU 官方 [v4.1.3 Release](https://github.com/SukiSU-Ultra/SukiSU-Ultra/releases/tag/v4.1.3) 下载 Manager APK。本项目验收的是 Manager/驱动代号 `40856`，不要直接改用未经本项目验证的新版本；
5. 把自己的原厂 `boot_a.bin` 复制到电脑并另外备份一份。

把 boot 镜像和 Manager APK 放进 `C:\platform-tools`。在该文件夹空白处按住 Shift 并单击右键，选择“在此处打开 PowerShell 窗口”。

### 3. 在 Android 中核对设备

连接设备，接受屏幕上的 USB 调试授权，然后运行：

```powershell
.\adb.exe devices
.\adb.exe shell getprop ro.build.fingerprint
.\adb.exe shell getprop ro.boot.slot_suffix
```

你应看到：

```text
设备状态       device
系统指纹       ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys
当前槽位       _a
```

出现 `unauthorized` 时，在设备屏幕上重新允许 USB 调试。指纹或槽位不一致时停止。

### 4. 核对下载的 boot

在 PowerShell 运行：

```powershell
(Get-Item .\w200ds-sukisu-v0.2.0-boot.img).Length
(Get-FileHash .\w200ds-sukisu-v0.2.0-boot.img -Algorithm SHA256).Hash.ToLowerInvariant()
```

结果必须完全等于：

```text
大小       67108864
SHA-256    a4e2fc24488ce806ce5282ee438ca1d8761c635edb1d02a20e7fa4cd98320e3e
```

不一致就重新从 GitHub Release 下载，不要刷入。

### 5. 写入 SukiSU boot

先进入 bootloader：

```powershell
.\adb.exe reboot bootloader
.\fastboot.exe devices
.\fastboot.exe getvar current-slot
.\fastboot.exe getvar is-userspace
.\fastboot.exe getvar partition-size:boot_a
```

继续前必须看到：

```text
current-slot: a
is-userspace: no
partition-size:boot_a: 0x4000000
```

只执行下面两条：

```powershell
.\fastboot.exe flash boot_a .\w200ds-sukisu-v0.2.0-boot.img
.\fastboot.exe reboot
```

只写一次 `boot_a`。不要使用 `fastboot boot`，不要切换槽位，不要擦除或写入其他分区。命令报错时停止，不要循环重试。

### 6. 检查首次启动

等待 Android 完整进入桌面，再运行：

```powershell
.\adb.exe wait-for-device
.\adb.exe shell getprop sys.boot_completed
.\adb.exe shell uname -a
.\adb.exe shell getprop ro.boot.slot_suffix
.\adb.exe shell getenforce
```

正确结果应包含：

- `sys.boot_completed` 为 `1`；
- `uname -a` 中有 `v0.2ga1@w200ds`；
- 槽位为 `_a`；
- SELinux 为 `Enforcing`。

### 7. 安装 SukiSU Manager

可以在设备文件管理器中打开 APK，也可以在 PowerShell 运行：

```powershell
.\adb.exe install -r .\你的-SukiSU-Manager-v4.1.3.apk
```

打开 Manager，确认它显示内核/驱动正在工作，代号为 `40856`。普通使用保持 **ADB Root 关闭**。不要同时安装或保留另一套 Magisk/KernelSU Root 实现。

此时 SukiSU Root 已经完成。模块不是启动 SukiSU 的必需条件。

### 8. 到这里，官方安装已经完成

Manager 正常识别 `40856` 驱动后，W200DS SukiSU 的基础安装已经完成。项目交付范围是：

- W200DS 内核源码、构建脚本和发布 boot；
- SukiSU 内核驱动及其官方 Manager；
- 安装、校验、回退和工程复现文档。

第三方模块、应用列表、模块配置、账号、keybox、认证数据和个人日志属于使用者的私人环境，类似系统出厂设置之外的个人文件。它们不属于本项目的安装步骤或发布验收内容，本项目也不会要求使用者公开完整模块列表、应用列表或无关截图。

报告内核问题时，应先关闭第三方模块复现；只有确认与某个模块直接相关时，才提供该模块名称、官方来源、版本和经过脱敏的最小日志。

### 9. 可选生态入口

以下项目是社区中常见的基础设施或功能入口，不是 W200DS SukiSU 的必装组件。本项目不规定组合、版本或安装顺序，也不捆绑、镜像或代替其维护者提供下载。

| 项目 | 作用 | 适合谁 |
|---|---|---|
| [Zygisk Next](https://github.com/Dr-TSNG/ZygiskNext) | 为 KernelSU/SukiSU 提供 Zygisk API | 只有准备使用明确依赖 Zygisk 的模块时；设备只能保留一种 Root 实现和一个 Zygisk 提供者 |
| [LSPosed](https://github.com/LSPosed/LSPosed) | 提供 ART Hook/Xposed 框架 | 只有确实使用 Xposed 模块时；它依赖可用的 Zygisk，作用域应按应用选择 |
| [LiteGApps](https://github.com/litegapps/litegapps) | 为缺少 Google 服务的系统提供精简、systemless GApps | 只有系统确实缺少且使用者需要 Google 服务时；必须自行核对 Android 13、arm64 和项目官方说明 |

部分模块需要修改 `/system` 视图。按 [KernelSU 模块指南](https://kernelsu.org/guide/module.html)，这类模块需要兼容的 metamodule/挂载实现；只运行脚本、`sepolicy` 或 `system.prop` 的模块不一定需要。选择时遵循当前 [metamodule 指南](https://kernelsu.org/guide/metamodule.html)，同一设备不要同时安装多个挂载提供者。

只从项目维护者的官方仓库或 Release 获取组件，先阅读其要求和回退说明。第三方组件更新可能改变兼容性；遇到问题应按对应项目的支持渠道处理。

### 10. 出现问题时怎么做

如果刷入后无法正常进入 Android：

1. 停止重复开机和重复刷写；
2. 使用你已经验证过的恢复包入口进入真正的 bootloader 或 recovery；
3. 只把与当前固件匹配的原厂 `boot_a.bin` 写回 `boot_a`：

```powershell
.\fastboot.exe flash boot_a .\boot_a.bin
.\fastboot.exe reboot
```

不要为了回退内核而全量刷恢复包，也不要执行 erase、format、`set_active` 或写其他槽位。无法进入 bootloader 时停止操作，使用原恢复包的既有救援流程或寻求 W200DS 专用帮助。

如果能开机但某个功能异常，先在 Manager 中关闭最后安装的模块并重启。一次只改变一个模块。

### 11. 完成检查

- [ ] 固件指纹完全匹配；
- [ ] A 槽、`boot_a=0x4000000`；
- [ ] boot 大小和 SHA-256 完全匹配；
- [ ] 只写入一次 `boot_a`；
- [ ] `v0.2ga1@w200ds`、`_a`、`Enforcing`、`boot_completed=1`；
- [ ] Manager v4.1.3 / 40856 正常；
- [ ] ADB Root 默认关闭；
- [ ] 原厂 `boot_a.bin` 仍保存在电脑外。

需要了解内核构建、OEM boot、ABI、Magisk 迁移或其他固件适配时，阅读 [W200DS SukiSU 移植 Helper](PORTING-HELPER.md)。

## English

### 1. Confirm compatibility

Proceed only when every item matches:

- ZTE W200DS / P720P01;
- fingerprint `ZTE/CN_P720P01/P720P01:13/TP1A.220624.014/20250218.231611:user/release-keys`;
- Android 13, arm64, active slot A;
- bootloader already unlocked with the recovery package;
- a known-good stock `boot_a.bin` is stored off-device; and
- Android boots normally with USB debugging enabled.

Other firmware, slots, and models are untested.

### 2. Prepare the files

1. Use a stable USB cable and charge the device.
2. Download and extract [Android Platform Tools](https://developer.android.com/tools/releases/platform-tools), for example to `C:\platform-tools`.
3. Download `w200ds-sukisu-v0.2.0-boot.img` and `SHA256SUMS` from the project [v0.2.0 Release](https://github.com/WiteTanuki/W200DS-SukiSU-Kernel/releases/tag/v0.2.0).
4. Download the Manager APK from the official SukiSU [v4.1.3 Release](https://github.com/SukiSU-Ultra/SukiSU-Ultra/releases/tag/v4.1.3). The tested Manager/driver code is `40856`; do not automatically substitute an untested newer release.
5. Keep another backup of your known-good stock `boot_a.bin`.

Put the boot image and APK in the Platform Tools directory and open PowerShell there.

### 3. Verify Android and the image

Accept the USB-debugging prompt and run:

```powershell
.\adb.exe devices
.\adb.exe shell getprop ro.build.fingerprint
.\adb.exe shell getprop ro.boot.slot_suffix
(Get-Item .\w200ds-sukisu-v0.2.0-boot.img).Length
(Get-FileHash .\w200ds-sukisu-v0.2.0-boot.img -Algorithm SHA256).Hash.ToLowerInvariant()
```

Require device state `device`, the exact fingerprint above, slot `_a`, size `67108864`, and SHA-256:

```text
a4e2fc24488ce806ce5282ee438ca1d8761c635edb1d02a20e7fa4cd98320e3e
```

Stop if any value differs.

### 4. Flash only boot_a

```powershell
.\adb.exe reboot bootloader
.\fastboot.exe devices
.\fastboot.exe getvar current-slot
.\fastboot.exe getvar is-userspace
.\fastboot.exe getvar partition-size:boot_a
```

Require `a`, `no`, and `0x4000000`, then run exactly once:

```powershell
.\fastboot.exe flash boot_a .\w200ds-sukisu-v0.2.0-boot.img
.\fastboot.exe reboot
```

Do not use `fastboot boot`, switch slots, erase partitions, write another partition, or automate retries.

### 5. Verify the first boot

After Android reaches the home screen:

```powershell
.\adb.exe wait-for-device
.\adb.exe shell getprop sys.boot_completed
.\adb.exe shell uname -a
.\adb.exe shell getprop ro.boot.slot_suffix
.\adb.exe shell getenforce
```

Require `1`, `v0.2ga1@w200ds`, `_a`, and `Enforcing`.

Install the v4.1.3 Manager APK with the file manager or:

```powershell
.\adb.exe install -r .\your-SukiSU-Manager-v4.1.3.apk
```

Confirm that Manager reports a working `40856` driver. Leave ADB Root disabled. Do not keep another Magisk/KernelSU root implementation alongside SukiSU.

### 6. The official installation ends here

Once Manager recognizes the working `40856` driver, the base W200DS SukiSU installation is complete. This project delivers:

- W200DS kernel source, build scripts, and the release boot image;
- the SukiSU kernel driver and its official Manager; and
- installation, verification, rollback, and build-reproduction documentation.

Third-party modules, app lists, module settings, accounts, keyboxes, attestation data, and personal logs belong to the user's private environment, like personal files added after a factory setup. They are not installation steps or release acceptance data. This project does not ask users to publish complete module or app lists, unrelated screenshots, or private configuration.

Before reporting a kernel issue, reproduce it with third-party modules disabled. Name a module, its official source, version, and a sanitized minimal log only when the problem is directly tied to that module.

### 7. Optional ecosystem entry points

These popular projects provide infrastructure or optional features. None is required to install W200DS SukiSU. This project does not prescribe a stack, version, or installation order, and does not bundle or mirror their downloads.

| Project | Purpose | Consider it when |
|---|---|---|
| [Zygisk Next](https://github.com/Dr-TSNG/ZygiskNext) | Provides the Zygisk API on KernelSU/SukiSU | A chosen module explicitly requires Zygisk; keep one root implementation and one Zygisk provider |
| [LSPosed](https://github.com/LSPosed/LSPosed) | Provides an ART-hooking/Xposed framework | You actually use Xposed modules; it requires working Zygisk and scopes should be selected per app |
| [LiteGApps](https://github.com/litegapps/litegapps) | Provides compact, systemless Google Apps for systems without Google services | The system lacks and the user needs Google services; independently confirm Android 13, arm64, and the maintainer's current instructions |

Modules that change the `/system` view need a compatible metamodule or mount implementation according to the [KernelSU module guide](https://kernelsu.org/guide/module.html). Modules limited to scripts, `sepolicy`, or `system.prop` may not need one. Follow the current [metamodule guide](https://kernelsu.org/guide/metamodule.html) and never install multiple mount providers together.

Download only from the maintainer's official repository or Releases and read its requirements and rollback instructions first. Third-party updates can change compatibility; use the component maintainer's support channel for component-specific problems.

### 8. Roll back if Android does not boot

Stop repeated boots and writes. Enter the real bootloader or recovery through your already-tested recovery-package path, then restore only the matching stock boot:

```powershell
.\fastboot.exe flash boot_a .\boot_a.bin
.\fastboot.exe reboot
```

Do not full-flash the recovery package merely to roll back the kernel. Do not erase, format, use `set_active`, or write another slot. If the bootloader is unreachable, stop and use the established recovery-package rescue procedure or seek W200DS-specific help.

For source builds, OEM boot layout, ABI work, Magisk migration, and troubleshooting, see the [W200DS SukiSU Porting Helper](PORTING-HELPER.md).
