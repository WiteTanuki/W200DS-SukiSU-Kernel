# 参与贡献 / Contributing

[简体中文](#简体中文) | [English](#english)

## 简体中文

提交应便于审查，并对应一个已证实的 W200DS 问题。

1. 基于当前公开分支开展工作，并说明已测试的固件。
2. 内核补丁遵循 Linux 内核代码风格，并运行 `scripts/checkpatch.pl`。
3. 保留上游作者信息、SPDX 标识和提交来源。
4. 适用 Android Common Kernel 规则时，使用 `UPSTREAM:`、`BACKPORT:`、`FROMGIT:` 或 `ANDROID:` 主题前缀。
5. 使用 `git commit -s` 添加真实的 `Signed-off-by:`，以认证 Developer's Certificate of Origin。
6. 不要提交专有二进制、私有模块、固件、分区转储、设备标识、凭据或证明数据。
7. 只披露复现问题所必需的信息：仅列出相关模块和版本；目标应用身份与问题无关时使用“目标应用”等通用称呼；不要附带个人应用清单、无关截图或完整设备日志。

影响启动、KernelSU ABI、OEM 模块 ABI、安全钩子或 boot 镜像的修改，必须提供聚焦的测试方案和恢复路径。避免无关重构和推测性的兼容层。

---

## English

Keep changes reviewable and tied to a demonstrated W200DS problem.

1. Base work on the current public branch and describe the tested firmware.
2. Follow Linux kernel coding style and run `scripts/checkpatch.pl` for kernel
   patches.
3. Preserve upstream authorship, SPDX identifiers and commit provenance.
4. Use `UPSTREAM:`, `BACKPORT:`, `FROMGIT:` or `ANDROID:` subjects where the
   Android Common Kernel guidance applies.
5. Add a `Signed-off-by:` line (`git commit -s`) to certify the Developer's
   Certificate of Origin.
6. Do not submit proprietary blobs, private modules, firmware, partition dumps,
   device identifiers, credentials or attestation data.
7. Disclose only what is necessary to reproduce the issue: list only relevant
   modules and versions; use a generic name such as “target app” when its identity
   is irrelevant; do not attach personal app inventories, unrelated screenshots
   or complete device logs.

A change that affects boot, KernelSU ABI, the OEM module ABI, security hooks or
the boot image must include a focused test plan and a recovery path. Avoid
unrelated refactors and speculative compatibility layers.
