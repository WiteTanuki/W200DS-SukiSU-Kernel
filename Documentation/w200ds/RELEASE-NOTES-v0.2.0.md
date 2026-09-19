# W200DS v0.2.0 发布说明 / Release notes

[简体中文](#简体中文) | [English](#english)

## 简体中文

### 与 RC1 的关系

`v0.2.0` 没有修改 `v0.2.0-rc1` 的内核源码、配置、KernelPatch 运行时或 boot
ramdisk。GA Image 由已验收的 RC1 Image 等长替换发布标记生成；两个 boot 镜像仅有
4 个字节不同，偏移均位于内核中的两处发布标记。

### 固定产物

| 文件 | 大小 | SHA-256 |
|---|---:|---|
| `w200ds-sukisu-v0.2.0-Image` | 39,854,288 | `19532e425e2893cdd983df069db48ec90699ace4c065cf745af3a56ca3ca90b7` |
| `w200ds-sukisu-v0.2.0-boot.img` | 67,108,864 | `a4e2fc24488ce806ce5282ee438ca1d8761c635edb1d02a20e7fa4cd98320e3e` |

运行标记为 `v0.2ga1@w200ds`。两次独立提升与打包产生了逐字节一致的输出；重解包后，
GA boot 的 ramdisk 与 RC1 boot 完全一致。

### 验收状态

RC1 已在唯一受测设备完成模块加载、3 次普通重启、2 次冷启动、超过 72 小时运行和
硬件功能验收。GA 还完成了主机侧等价性、可复现性和一次精确 `boot_a` 采用；启动后
`v0.2ga1@w200ds`、A 槽、normal、SELinux Enforcing、普通 ADB Shell、SukiSU Manager、
既有活动模块及 ADB Root 默认关闭均通过。没有自动重试、自动回退或其他分区写入。

## English

### Relationship to RC1

`v0.2.0` changes no kernel source, configuration, KernelPatch runtime or boot
ramdisk from `v0.2.0-rc1`. The GA Image is produced by an equal-length release
marker replacement in the accepted RC1 Image. The two boot images differ by
only four bytes, all within the two kernel release-marker occurrences.

### Fixed artifacts

| File | Size | SHA-256 |
|---|---:|---|
| `w200ds-sukisu-v0.2.0-Image` | 39,854,288 | `19532e425e2893cdd983df069db48ec90699ace4c065cf745af3a56ca3ca90b7` |
| `w200ds-sukisu-v0.2.0-boot.img` | 67,108,864 | `a4e2fc24488ce806ce5282ee438ca1d8761c635edb1d02a20e7fa4cd98320e3e` |

The runtime marker is `v0.2ga1@w200ds`. Two independent promotion and repack
runs produced byte-identical outputs. Re-unpacking also showed that the GA and
RC1 boot ramdisks are byte-identical.

### Acceptance status

RC1 passed module loading, three normal reboots, two cold boots, more than 72
hours of operation and hardware checks on the one tested device. GA also passed
host equivalence, reproducibility and one exact `boot_a` adoption. Post-boot
checks passed for `v0.2ga1@w200ds`, slot A, normal mode, SELinux Enforcing,
ordinary ADB Shell, SukiSU Manager, the existing active modules and ADB Root
disabled by default. There was no automatic retry, rollback or other-partition
write.
