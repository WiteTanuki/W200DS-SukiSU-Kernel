# W200DS 恢复说明 / Recovery

[简体中文](#简体中文) | [English](#english)

## 简体中文

请在安装前准备恢复条件：保留属于你自己的、确认可用的原厂 boot、匹配的主机工具，并确保设备电量充足。本仓库有意不包含 OEM 固件或紧急下载加载器。

如果 Android 无法启动：

1. 停止反复开机，也不要试验其他分区。
2. 优先尝试设备正常的 Bootloader 或 Recovery 路径。
3. 仅使用与当前固件完全匹配、确认可用的镜像恢复 `boot_a`。
4. 如果设备无法进入 Bootloader，只能采用事先在该设备上验证过的专用应急流程，并独立核验其工具。

W200DS 的 OEM Recovery 可以作为受限入口确认设备仍可响应，但其“通过 ADB/SD 卡
安装更新”面向签名升级包，不能替代 `boot_a` 的精确镜像恢复。已验证恢复基线只覆盖
A 槽；不要把 B 槽、切槽或紧急下载模式当作通用试错路径。

不要执行 erase、format、切换活动槽位，也不要刷入其他型号的镜像。USB 连接成功并不能证明镜像兼容。

---

## English

Prepare recovery before installation: keep your own known-good stock boot,
matching host tools and a charged device. This repository intentionally does
not bundle OEM firmware or emergency download loaders.

If Android does not start:

1. Stop repeated boot attempts and do not experiment with other partitions.
2. Try the device's normal bootloader/recovery path.
3. Restore only `boot_a` with the known-good image for the exact firmware.
4. If the device cannot reach the bootloader, use only a previously validated
   device-specific emergency procedure and its independently verified tools.

The W200DS OEM Recovery can serve as a limited proof that the device still
responds, but its ADB/SD-card update options expect signed update packages and
do not replace an exact `boot_a` image restore. The validated baseline covers
slot A only; do not use slot B, slot switching or emergency-download mode as
generic trial-and-error paths.

Never erase, format, change the active slot or flash images from another model.
Do not assume that a successful USB connection proves the image is compatible.
