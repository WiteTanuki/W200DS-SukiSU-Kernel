# 源码版本记录 / Source revisions

[简体中文](#简体中文) | [English](#english)

## 简体中文

公开发布仓库是一份干净的源码快照，有意排除了私有开发历史、日志、设备标识和恢复资产。

| 组件 | 固定版本 / 标识 |
|---|---|
| v0.2.0-rc1 内核源码基线 | `75982acf2d1a5708a654250b3b1f2462415fc1a9` |
| 公开发布分支 | `main` |
| SukiSU 集成基线 | `b1d534bc41941b2c818d7a1a1dac341e4aabfc2d` |
| KernelPatch 运行时源码 | `1b0fddd090bc724ae5798e684acdcd1bcd31071f` |
| SUSFS 现代接口参考 | `ab4c23cfc7cb26821abb7a9d2071206713c070fe` |
| SUSFS 5.4 系谱参考 | `76affd70` |
| 内核版本 | `5.4.276-android12-9-g83f519fbd509` |
| OEM 模块 ABI | 170 个已构建模块；205 个 OEM 模块；导入 CRC 10,324/10,324；`module_layout=0x2f279e7b` |
| OEM 公共模块证书 SHA-256 | `f4627fc26fb89481318cc658c440233943205cb6a8e71ecadad349e469b44ee7` |

最终发布清单会记录文档/脚本收尾后的标签提交；上表的源码基线是产生固定 Full LTO
Image 的内核树。导出源码文件保留上游版权与 `Signed-off-by` 记录。

---

## English

The public release repository is a clean snapshot; it intentionally excludes
private development history, logs, device identifiers and recovery assets.

| Component | Fixed revision / identity |
|---|---|
| v0.2.0-rc1 kernel source baseline | `75982acf2d1a5708a654250b3b1f2462415fc1a9` |
| Public release branch | `main` |
| SukiSU integration baseline | `b1d534bc41941b2c818d7a1a1dac341e4aabfc2d` |
| KernelPatch runtime source | `1b0fddd090bc724ae5798e684acdcd1bcd31071f` |
| SUSFS modern contract reference | `ab4c23cfc7cb26821abb7a9d2071206713c070fe` |
| SUSFS 5.4 lineage reference | `76affd70` |
| Kernel release | `5.4.276-android12-9-g83f519fbd509` |
| OEM module ABI | 170 built modules; 205 OEM modules; 10,324/10,324 imported CRCs; `module_layout=0x2f279e7b` |
| OEM public module certificate SHA-256 | `f4627fc26fb89481318cc658c440233943205cb6a8e71ecadad349e469b44ee7` |

The final manifest records the tag commit after documentation and wrapper
finalization. The source baseline above is the kernel tree that produced the
fixed Full LTO Image. Upstream copyright and Signed-off-by records remain in the
exported source files.
