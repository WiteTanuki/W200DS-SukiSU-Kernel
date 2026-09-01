# 源码版本记录 / Source revisions

[简体中文](#简体中文) | [English](#english)

## 简体中文

公开发布仓库是一份干净的源码快照，有意排除了私有开发历史、日志、设备标识和恢复资产。

| 组件 | 固定版本 / 标识 |
|---|---|
| 已测试的内核集成树 | `df118757b7642c0c3c9e22b82a59d2dbfb03f4af` |
| 测试所用源码分支 | `codex/r2-kernel-refactor` |
| SukiSU 集成基线 | `b1d534bc41941b2c818d7a1a1dac341e4aabfc2d` |
| KernelPatch 运行时源码 | `1b0fddd090bc724ae5798e684acdcd1bcd31071f` |
| SUSFS 现代接口参考 | `ab4c23cfc7cb26821abb7a9d2071206713c070fe` |
| SUSFS 5.4 系谱参考 | `76affd70` |
| 内核版本 | `5.4.276-android12-9-g83f519fbd509` |
| OEM 模块 ABI | 170 个已构建模块；205 个 OEM 模块；导入 CRC 10,324/10,324；`module_layout=0x2f279e7b` |
| OEM 公共模块证书 SHA-256 | `f4627fc26fb89481318cc658c440233943205cb6a8e71ecadad349e469b44ee7` |

此快照提交后生成的发布清单会记录干净的发布提交和标签。导出源码文件保留上游版权与 `Signed-off-by` 记录。

---

## English

The public release repository is a clean snapshot; it intentionally excludes
private development history, logs, device identifiers and recovery assets.

| Component | Fixed revision / identity |
|---|---|
| Tested kernel integration tree | `df118757b7642c0c3c9e22b82a59d2dbfb03f4af` |
| Source branch used for testing | `codex/r2-kernel-refactor` |
| SukiSU integration baseline | `b1d534bc41941b2c818d7a1a1dac341e4aabfc2d` |
| KernelPatch runtime source | `1b0fddd090bc724ae5798e684acdcd1bcd31071f` |
| SUSFS modern contract reference | `ab4c23cfc7cb26821abb7a9d2071206713c070fe` |
| SUSFS 5.4 lineage reference | `76affd70` |
| Kernel release | `5.4.276-android12-9-g83f519fbd509` |
| OEM module ABI | 170 built modules; 205 OEM modules; 10,324/10,324 imported CRCs; `module_layout=0x2f279e7b` |
| OEM public module certificate SHA-256 | `f4627fc26fb89481318cc658c440233943205cb6a8e71ecadad349e469b44ee7` |

The clean release commit and tag are recorded in the release manifest generated
after this snapshot is committed. Upstream copyright and Signed-off-by records
remain in the exported source files.
