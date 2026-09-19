# 已知限制 / Known limitations

[简体中文](#简体中文) | [English](#english)

## 简体中文

- 仅支持列表中指定的 W200DS / P720P01 Android 13 固件。
- Root 隐藏只能尽力而为，无法保证通过所有应用的本地检查或未来的远程完整性策略。
- SELinux 保持 Enforcing。Manager 的“隐藏 SELinux 修改”只启用 Linux 5.4 原生的
  只读 clean-view，不改变实际 enforcing/policy 状态，也不能隐藏所有 SELinux 侧信道。
- 此版本实现 KernelSU ADB Root，但默认关闭；启用后 ADB 安全边界会扩大，仅应在
  明确的短时维护窗口使用，结束后关闭。
- 已验证 KernelPatch 运行时 KPM 的加载与卸载路径，但此版本没有嵌入 KPM，也不提供 KPM 载荷。
- 仅替换内核无法解决 OEM `/system/bin/su` 与 `/system/bin/sh` 的别名行为；隐藏共享 inode 也可能影响 Shell。
- 验收 boot 保留了来自受支持 OEM boot 镜像且未经修改的 ramdisk。二进制发布的再分发规则需要由用户自行评估。
- 模块兼容性取决于具体模块及其配置。私有模块和第三方模块不属于本仓库的支持范围。
- 未验证系统 OTA 更新；更新固件前应恢复匹配的原厂 boot，并重新核对支持指纹。
- 已验证的恢复范围限于 A 槽 `boot_a`。Recovery 菜单中的“通过 ADB/SD 卡安装更新”
  不等同于 boot 镜像恢复；本仓库不提供 B 槽或紧急下载模式的通用操作方案。

---

## English

- Only the listed W200DS / P720P01 Android 13 firmware is supported.
- Root hiding is best-effort and cannot guarantee acceptance by every app or
  future remote integrity policy.
- SELinux remains Enforcing. The Manager's **Hide SELinux modifications** toggle
  enables only a native Linux 5.4 read-only clean view; it changes neither the
  real enforcing/policy state nor every SELinux side channel.
- KernelSU ADB Root is implemented but disabled by default. Enabling it expands
  the ADB trust boundary and should be limited to an explicit maintenance window.
- KernelPatch runtime KPM load/unload was validated, but this release embeds no
  KPM and ships no KPM payload.
- The OEM `/system/bin/su` and `/system/bin/sh` alias behavior is not solved by
  replacing the kernel. Hiding the shared inode can also affect the shell.
- The accepted boot contains an unchanged ramdisk derived from the supported
  OEM boot image. Users must assess redistribution rules for the binary release.
- Module compatibility depends on the module and its configuration. Private or
  third-party modules are outside this repository.
- System OTA updates are untested. Restore the matching stock boot and re-check
  the supported fingerprint before applying firmware updates.
- The validated recovery scope is limited to slot A `boot_a`. Recovery menu
  options for ADB/SD-card updates are not boot-image recovery, and this
  repository provides no generic slot-B or emergency-download procedure.
