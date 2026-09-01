# 第三方声明与来源 / Third-party notices and provenance

[简体中文](#简体中文) | [English](#english)

## 简体中文

本仓库汇集了带有不同上游声明的工作，不替代或覆盖任何原声明。

### Linux / Android Common Kernel 与厂商衍生内核源码

内核源码树保留 `COPYING`、`LICENSES/` 和逐文件 SPDX 标识。设备集成验收基于提交 `df118757b7642c0c3c9e22b82a59d2dbfb03f4af`。

### SukiSU / KernelSU 衍生集成

集成基线记录为 `b1d534bc41941b2c818d7a1a1dac341e4aabfc2d`。其源码与声明位于 `drivers/kernelsu/` 及相关集成文件中。

### KernelPatch 运行时

固定运行时快照位于 `tools/kernelpatch-runtime/`，版本为 `1b0fddd090bc724ae5798e684acdcd1bcd31071f`。该组件声明 GPL-2.0，并保留自身的 `LICENSE`、README 和致谢。

### SUSFS 衍生代码

5.4 集成可追溯至已记录的 `76affd70` 系谱，现代接口参考为 `ab4c23cfc7cb26821abb7a9d2071206713c070fe`。其上游 GPL-3.0-or-later 许可证文本保留在 `LICENSES/third-party/SUSFS-GPL-3.0-or-later.txt`。

GPL-3.0-or-later 的 SUSFS 衍生代码与 GPL-2.0-only 内核发布并存，已被识别为需要相关著作权人澄清的来源和兼容性问题。本候选版本保留组件边界和声明，不主张对任何组件重新许可，并如实记录尚未解决的澄清事项。若作者提供明确许可或兼容系谱，发布者应更新本声明。

### 未包含的材料

本源码仓库不包含 Manager APK、第三方模块、私有 LSPosed 构建、OEM 固件、分区转储、FDL 加载器、设备日志、序列号、凭据或私钥。

---

## English

This repository aggregates work with different upstream notices. It does not
replace or override them.

### Linux / Android Common Kernel and vendor-derived kernel source

The kernel tree retains `COPYING`, `LICENSES/` and per-file SPDX identifiers.
The accepted device integration was tested from commit
`df118757b7642c0c3c9e22b82a59d2dbfb03f4af`.

### SukiSU / KernelSU-derived integration

The integrated baseline is recorded as
`b1d534bc41941b2c818d7a1a1dac341e4aabfc2d`. Its source and notices are present
under `drivers/kernelsu/` and related integration files.

### KernelPatch runtime

The fixed runtime snapshot is under `tools/kernelpatch-runtime/`, revision
`1b0fddd090bc724ae5798e684acdcd1bcd31071f`. It declares GPL-2.0 and retains its
own `LICENSE`, README and credits.

### SUSFS-derived code

The 5.4 integration traces to the recorded `76affd70` lineage, with modern
contract reference `ab4c23cfc7cb26821abb7a9d2071206713c070fe`. Its upstream
GPL-3.0-or-later license text is retained at
`LICENSES/third-party/SUSFS-GPL-3.0-or-later.txt`.

The coexistence of GPL-3.0-or-later SUSFS-derived code with a GPL-2.0-only
kernel distribution has been identified as a provenance/compatibility question
for the relevant copyright holders. This release candidate preserves the
component boundary and notices, makes no claim to relicense either component,
and records the unresolved clarification instead of concealing it. A publisher
should update this notice if the authors provide a definitive permission or
compatible lineage.

### Excluded material

No Manager APK, third-party module, private LSPosed build, OEM firmware,
partition dump, FDL loader, device log, serial number, credential or private key
is part of this source repository.
