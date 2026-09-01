# 已知限制 / Known limitations

[简体中文](#简体中文) | [English](#english)

## 简体中文

- 仅支持列表中指定的 W200DS / P720P01 Android 13 固件。
- Root 隐藏只能尽力而为，无法保证通过所有应用的本地检查或未来的远程完整性策略。
- SELinux 保持 Enforcing。此版本不支持 Manager 的“隐藏 SELinux 修改”；SELinux-hide 属于独立且已冻结的研究分支。
- 此版本未启用 KernelSU ADB Root。普通 ADB 默认仍是普通 Shell；用户在 Manager 中明确为 Shell 授权 root 的情况除外。
- 已验证 KernelPatch 运行时 KPM 的加载与卸载路径，但此版本没有嵌入 KPM，也不提供 KPM 载荷。
- 仅替换内核无法解决 OEM `/system/bin/su` 与 `/system/bin/sh` 的别名行为；隐藏共享 inode 也可能影响 Shell。
- 验收 boot 保留了来自受支持 OEM boot 镜像且未经修改的 ramdisk。二进制发布的再分发规则需要由用户自行评估。
- 模块兼容性取决于具体模块及其配置。私有模块和第三方模块不属于本仓库的支持范围。

---

## English

- Only the listed W200DS / P720P01 Android 13 firmware is supported.
- Root hiding is best-effort and cannot guarantee acceptance by every app or
  future remote integrity policy.
- SELinux remains Enforcing. The Manager's **Hide SELinux modifications**
  feature is not supported; SELinux-hide is a separate, frozen research track.
- KernelSU ADB Root is not enabled. Normal ADB remains an ordinary shell unless
  the user explicitly grants Shell root through the Manager.
- KernelPatch runtime KPM load/unload was validated, but this release embeds no
  KPM and ships no KPM payload.
- The OEM `/system/bin/su` and `/system/bin/sh` alias behavior is not solved by
  replacing the kernel. Hiding the shared inode can also affect the shell.
- The accepted boot contains an unchanged ramdisk derived from the supported
  OEM boot image. Users must assess redistribution rules for the binary release.
- Module compatibility depends on the module and its configuration. Private or
  third-party modules are outside this repository.
