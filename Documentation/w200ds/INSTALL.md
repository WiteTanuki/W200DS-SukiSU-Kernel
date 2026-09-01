# W200DS 安装说明 / Installation

[简体中文](#简体中文) | [English](#english)

## 简体中文

这是面向特定设备的发布版本。安装前请先阅读恢复说明，并在设备外保存一份属于你自己的、确认可用的原厂 `boot_a`。

### 前置条件

- 设备为 ZTE W200DS / P720P01，且固件指纹与支持列表完全一致；
- Bootloader 已解锁；
- 当前活动槽位为 A，且已进入真正的 Bootloader（`is-userspace: no`）；
- boot 分区大小为 `0x4000000`（64 MiB）；
- 发布镜像的 SHA-256 已与 `SHA256SUMS` 核对一致。

任一标识不一致时都不要继续。不要使用 `fastboot boot`，不要刷写其他槽位，也不要写入任何其他分区。

### 最小 Bootloader 流程

```text
adb reboot bootloader
fastboot getvar current-slot
fastboot getvar is-userspace
fastboot getvar partition-size:boot_a
fastboot flash boot_a w200ds-sukisu-v0.2.0-rc1-boot.img
fastboot reboot
```

以上命令仅作为操作说明，并非自动安装器。请自行确认仅连接了一台目标设备。Android 启动后，先确认发布标记、A 槽、SELinux Enforcing 和 SukiSU Manager 状态，再逐个启用模块。

---

## English

This is a device-specific release. Read the recovery guide first and preserve a
known-good copy of your own stock `boot_a` outside the device.

### Preconditions

- ZTE W200DS / P720P01 on the exact supported firmware fingerprint;
- unlocked bootloader;
- slot A active and a real bootloader (`is-userspace: no`);
- boot partition size `0x4000000` (64 MiB);
- release image SHA-256 verified against `SHA256SUMS`.

Do not continue if any identity differs. Do not use `fastboot boot`, flash
another slot or write any other partition.

### Minimal bootloader flow

```text
adb reboot bootloader
fastboot getvar current-slot
fastboot getvar is-userspace
fastboot getvar partition-size:boot_a
fastboot flash boot_a w200ds-sukisu-v0.2.0-rc1-boot.img
fastboot reboot
```

The commands are documentation, not an automated installer. Verify the one
connected device yourself. After Android starts, confirm the release marker,
slot A, SELinux Enforcing and SukiSU Manager status before enabling modules.
