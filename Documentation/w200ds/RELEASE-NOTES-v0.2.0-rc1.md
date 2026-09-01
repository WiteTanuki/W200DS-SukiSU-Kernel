# v0.2.0-rc1 发布说明 / Release notes

[简体中文](#简体中文) | [English](#english)

> 当前状态：主机发布验收与唯一受测设备采用均已通过。设备仅写入 `boot_a` 一次，
> 启动后确认 `v0.2rc1@w200ds`、A 槽、normal、Enforcing、普通 ADB Shell、
> `boot_completed=1` 以及 Manager 工作状态。

## 简体中文

### 相对 v0.1.0-rc1 的主要变化

- 将 KernelSU ADB Root 完整接入 Linux 5.4 构建与 Manager 能力探测；默认关闭。
- 增加 Linux 5.4 原生 SELinux 只读 clean-view，为 Manager“隐藏 SELinux 修改”提供
  明确、局部且不削弱真实策略的后端。
- 保留 SukiSU / KernelSU、SUSFS 2.2.0 inline hook、SukiSU 模块框架和 runtime KPM；
  embedded KPM 数量仍为 0。
- 固定可复现 Full LTO 构建身份，清理源码/输出绝对路径泄漏，并继续保持 OEM 模块
  ABI、公共证书和 CMS 验收。
- 更新发布包装脚本、唯一等长标记、构建文档、安装说明和隐私边界。

### 受测发布物身份

| 产物 | SHA-256 | 大小 |
|---|---|---:|
| 原始 Full LTO Image | `be5f744963f9eca5b7b84e37747cb191aabc28d4019d403e31f43f221edaf16e` | 39,672,320 |
| KernelPatch 后链接 Image | `4bf47964827f138c6941b0e0c90f8500763f3b7b9b2ac60e52770c9590152ff7` | 39,854,288 |
| `v0.2rc1@w200ds` Image | `cea4afc177e56e93c8fbc7ed794767595b6c0d826e7e40b2e2f99d514b3cebfb` | 39,854,288 |
| 受测 boot | `1cd91b2119848a674c3473a6a609926591c5e0b37d48bf2aa95fcd6af3fceb6c` | 67,108,864 |

Full LTO、170/170 重建模块、205/205 OEM 模块、10,324/10,324 OEM CRC、OEM
公共模块证书、KernelPatch `patched=true / extras=0`、boot 重解包和 stock ramdisk
逐字节保真均已通过。

### 设备验收结果

唯一设备与控制基线、真 bootloader、slot A、`is-userspace=no`、64 MiB `boot_a`
全部通过。候选写入与 reboot 各一次；启动后标记、normal、Enforcing、普通 Shell、
Manager 及新增功能支持均通过。自动重试、自动回退和其他分区写入均为 0。

---

## English

> Current state: host release acceptance and adoption on the one tested device
> both passed. `boot_a` was written once; post-boot checks confirmed
> `v0.2rc1@w200ds`, slot A, normal mode, Enforcing, ordinary ADB Shell,
> `boot_completed=1`, and a working Manager.

### Main changes from v0.1.0-rc1

- Complete KernelSU ADB Root build and Manager capability integration for Linux
  5.4; disabled by default.
- Add a native Linux 5.4 read-only SELinux clean view for the Manager's **Hide
  SELinux modifications** toggle without weakening real policy enforcement.
- Retain SukiSU / KernelSU, SUSFS 2.2.0 inline hooks, the SukiSU module framework
  and runtime KPM; embedded KPM count remains zero.
- Pin reproducible Full LTO output, remove build/source absolute-path disclosure,
  and retain OEM module ABI, public-certificate and CMS acceptance.
- Refresh release wrappers, the unique equal-length marker, build/install docs
  and privacy boundaries.

### Accepted release identities

| Artifact | SHA-256 | Bytes |
|---|---|---:|
| Raw Full LTO Image | `be5f744963f9eca5b7b84e37747cb191aabc28d4019d403e31f43f221edaf16e` | 39,672,320 |
| KernelPatch post-linked Image | `4bf47964827f138c6941b0e0c90f8500763f3b7b9b2ac60e52770c9590152ff7` | 39,854,288 |
| `v0.2rc1@w200ds` Image | `cea4afc177e56e93c8fbc7ed794767595b6c0d826e7e40b2e2f99d514b3cebfb` | 39,854,288 |
| Accepted boot | `1cd91b2119848a674c3473a6a609926591c5e0b37d48bf2aa95fcd6af3fceb6c` | 67,108,864 |

Full LTO, 170/170 rebuilt modules, 205/205 OEM modules, 10,324/10,324 OEM CRCs,
the OEM public module certificate, KernelPatch `patched=true / extras=0`, boot
re-unpack and byte-identical stock ramdisk checks all passed.

### Device acceptance result

The one-device/control baseline, real bootloader, slot A, `is-userspace=no` and
64 MiB `boot_a` checks passed. The candidate was written once and rebooted once;
the marker, normal mode, Enforcing, ordinary Shell, Manager and new feature
support passed. Automatic retries, automatic rollback and other-partition writes
were zero.
