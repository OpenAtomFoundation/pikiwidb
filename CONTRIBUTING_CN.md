# 贡献指南

> For the English version, see [CONTRIBUTING.md](CONTRIBUTING.md).

感谢你有兴趣为 PikiwiDB(Pika)贡献力量。本文档说明我们的代码贡献与评审流程,
其设计对标开源社区最佳实践(OpenSSF)。

## 行为准则

所有参与者都应遵守我们的[行为准则](CODE_OF_CONDUCT.md)。如发现不当行为,请 上报至

 `wuxianrong@360.cn`。

## 上报安全问题

请**不要**为安全漏洞创建公开 issue,请改为遵循 [SECURITY_CN.md](SECURITY_CN.md)
中的流程。

## 贡献方式

- 通过[issue 模板](.github/ISSUE_TEMPLATE/)上报缺陷或提出功能需求。
- 完善文档。
- 提交代码变更(缺陷修复、功能、测试、工具链)。

## 开发流程

1. **Fork** 仓库,并从合适的基础分支创建主题分支(新功能开发使用 `unstable`;
  定向修复使用 `3.5` / `4.0`)。
2. **签署 CLA**。贡献者在贡献被合并前必须签署贡献者许可协议
  (见 `[docs/cla/](docs/cla/)`)。
3. **进行修改**,遵循 `.clang-format` 与 `.clang-tidy` 强制的代码风格
  (提交前运行 `./format_code.sh`)。
4. 尽可能为新行为和缺陷修复**补充测试**。
5. 向正确的基础分支**发起 Pull Request**。

## 提交与来源要求

- 每个 commit 必须由经过身份认证的提交者完成。我们鼓励使用**签名提交**
(`git commit -S`)以增强来源可信度。
- 引入第三方代码时,须在 commit 备注中清晰标注其来源与许可证
(例如 `[External] Import from <project> <version>`),并确保其与 PikiwiDB 的
[许可证](LICENSE.md)兼容。

## Pull Request 要求

- **PR 标题**必须遵循 [Conventional Commits](https://www.conventionalcommits.org/)
规范并使用英文,由 `PR Title Checker` 工作流自动校验。允许的前缀:
`feat`、`fix`、`test`、`refactor`、`chore`、`upgrade`、`style`、`docs`、
`perf`、`build`、`ci`、`revert`。
- 合并前所有 **CI 检查必须通过**,包括:
  - 编译与集成测试(`Pika` 工作流)。
  - 静态/安全分析(`CodeQL` 工作流)。
- 每个 PR 都需要经过 **Maintainer 的评审与批准**。评审覆盖正确性、安全、性能与
兼容性。
- 在 PR 描述中关联对应 issue,使每次变更都具备可追溯的"上报 → 修复 → 合并"
记录。

## 评审与合并策略

- 评审由 [MAINTAINERS.md](MAINTAINERS.md) 中列出的 Maintainer 与 Committer 执行。
- 评审者从**安全、性能、兼容性**等维度评估变更。
- 当变更获得所需批准且所有 CI 门禁通过后方可合并。Maintainer 拥有最终的合并与
发布决策权。

## 依赖

依赖更新通过 [Dependabot](.github/dependabot.yml) 自动跟踪。请保持新依赖精简、
版本锁定且许可证兼容。

## 成为 Committer 或 Maintainer

晋升标准与提名流程详见 [MAINTAINERS.md](MAINTAINERS.md)。

## 疑问

如有治理或贡献相关疑问,请创建 issue 或联系 `wuxianrong@360.cn`。