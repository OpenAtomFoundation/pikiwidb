# 漏洞扫描处置记录（unstable 分支，2026-08-07）

来源：`unstable项目-漏洞列表-20260807155747.xlsx`，共 119 条。
处置分支：`fix/deps-security-scan-20260807`。

> 重要前提：本次扫描项全部落在仓库附带的 **codis 集群管理组件** 和 **tools/** 目录，
> **不涉及 PikiwiDB 的 C++ 存储引擎本体**。报告中的 `redis 4.4.4` 是组件识别错误（见误报表）。

## 一、已实际修复（升级依赖 + 构建验证通过）

| 模块 | 依赖 | 原版本 | 修复版本 | 覆盖 CVE |
|---|---|---|---|---|
| tools/pika_exporter | github.com/sirupsen/logrus | v1.9.0 | **v1.9.4** | CVE-2025-65637 |
| tools/pika_exporter | google.golang.org/protobuf | v1.33.0 | **v1.35.2** | CVE-2024-24786 相关 |
| tools/pika_exporter | golang.org/x/sys | v0.10.0 | **v0.33.0** | GHSA-p782-xgp4-8hr8 / GO-2022-0493 |
| tools/pika_keys_analysis | golang.org/x/sys | v0.14.0 | **v0.33.0** | 同上 |
| tools/codis2pika | golang.org/x/sys | v0.1.0 | **v0.33.0** | 同上 |
| codis | google.golang.org/grpc (replace) | v1.29.0 | **v1.56.3** | CVE-2023-32731 / CVE-2023-39325 / CVE-2023-44487 / GHSA-hrxh-6v49-42gf |
| tests/integration | github.com/redis/go-redis/v9 | v9.4.0 | **v9.7.3** | GO-2025-3540 |
| tools/pika_exporter | golang.org/x/net | (旧快照) | **v0.38.0** | CVE-2023-3978 / CVE-2023-45288 / CVE-2022-41723 / CVE-2024-24790 |
| tools/pika_exporter | golang.org/x/text | (旧快照) | **v0.23.0** | CVE-2022-32149 / CVE-2021-38561 |
| tools/pika_exporter | golang.org/x/crypto | (旧快照) | **v0.36.0** | CVE-2025-22869 |
| codis/cmd/fe/assets | bootstrap | 3.3.6 | **3.4.1** | CVE-2018-14041 / CVE-2018-14040 / CVE-2018-20676 / CVE-2018-20677 / CVE-2019-8331 / CVE-2016-10735 |
| codis/cmd/fe/assets | jquery | 2.1.4 | **3.7.1** | CVE-2020-11022 / CVE-2020-11023 / CVE-2019-11358 / CVE-2015-9251 / CVE-2023-43051 |
| codis/cmd/fe/assets | angular (AngularJS) | 1.4.8 | **1.8.3** | CVE-2019-10768 / CVE-2020-7676 / CVE-2022-25844 / CVE-2023-26116/26117/26118 |
| codis/cmd/fe/assets | highcharts | 4.1.10 | **9.3.3** | CVE-2021-29489（需 v9+）/ CVE-2018-20801（需 v6.1+） |

验证方式：各 Go 模块 `go build ./...` 通过；pika_exporter 经 `make build` 产出可执行文件；
codis `go mod verify` = all modules verified；Bootstrap 为 3.x 分支内纯安全补丁，dist 结构一致、零 API breaking。
前端 jQuery/AngularJS 为 vendored 到 git 的静态库（无构建步骤），已整目录替换为官方 npm tarball：
`node --check` 语法完整，index.html 全部引用文件解析正常，`angular-ui-bootstrap@0.14.3` peer 范围（无上限）兼容 AngularJS 1.8.3。
业务代码 `dashboard-fe.js` 经扫描无 jQuery 2→3 / AngularJS 1.4→1.8 的 breaking API 调用（`.success/.error`、`.andSelf/.size/.bind/.live` 等均为 0），
但无 live Codis 集群，**UI 回归未经人工验证**，合并前需在真实面板过一遍关键交互。

Highcharts 4.1.10 → 9.3.3 升级说明（2026-08-21 复扫后修复）：
`highcharts` 为 vendored 静态库，整目录替换为官方 npm tarball（9.3.3 主包 `highcharts.js` 与 `highcharts-more.js` 已从旧版 `lib/` 移到包根，`index.html` 脚本路径同步调整）。
`highcharts-ng@0.0.11` 封装通过 `new Highcharts[chartType](...)` 动态构造图表，经浏览器 smoke test 验证与 9.3.3 兼容：
`Highcharts.version=9.3.3`，`chart_ops` spline（highchart directive）与 `slots_charts` columnrange（`new Highcharts.Chart`，与 `renderSlotsCharts` 同路径）均正常渲染 SVG，
定时刷新推点、`destroy`/重建生命周期、以及 `dashboard-fe.js` 中已废弃的 `setOptions({global:{useUTC:false}})` 调用均无报错，浏览器控制台零错误，故未引入额外适配层。
smoke test 仅覆盖静态页面与 mock 数据，真实接口返回、权限流程与集群状态页仍需合并前在 live 面板人工回归。

## 二、误报 / 已在仓库中修复（报告版本与实际代码不符）

扫描器采集的是过时快照，以下依赖仓库**当前实际版本已是安全版**，报告所列老版本并不存在：

| 报告组件 | 报告版本 | 仓库实际版本 | 结论 |
|---|---|---|---|
| golang.org/x/net | 0.0.0-20190404… | codis 中 **v0.33.0** | 误报 |
| gopkg.in/yaml.v3 | 3.0.0-20200313… | 各处 **v3.0.1** | 误报 |
| golang.org/x/crypto | 0.0.0-20190123… | 不存在于任何 go.mod | 误报（组件不存在） |
| golang.org/x/text | 0.3.0 | 不存在于当前依赖树 | 误报 |
| rsc.io/pdf | 0.1.1 | 不存在于任何 go.mod | 误报（组件不存在） |
| toml 0.10.2 / TYPOSQUAT | — | 仓库用 BurntSushi/toml、pelletier/go-toml | 误报（组件识别错误） |
| **redis 4.4.4**（约 30 条） | 4.4.4 | PikiwiDB 为自研 C++ 引擎，无此组件 | 误报（组件识别错误） |
| angular / grpc 的 xDS/HTTP-server 类 | — | codis 不启用 gRPC server、无 xDS | 攻击面不适用 |

## 三、需人工评估（高风险，本次未改动）

codis 面板前端为 AngularJS 1.x 深度耦合的 SPA。jQuery、AngularJS、Highcharts 均已升到可修复范围内的最高安全版本，
仅剩 AngularJS 一项因框架 EOL 无法通过继续升级消除，留待前端评估：

| 组件 | 当前版本 | 情况 |
|---|---|---|
| angular (AngularJS) | 1.8.3（已升到 1.x 终版） | 1.x 已 EOL；1.8.3 是官方最后一个版本。最新复扫（2026-08-21）报告的 CVE-2024-21490（`ng-srcset` ReDoS）、CVE-2023-43004 **无 1.x 补丁版本**，不能通过继续升级 1.x 消除。短期缓解只能限制相关输入来自可信后端并做长度/格式校验，**仅降低攻击面，不等于 CVE 已修复**；根治需迁移到 Angular 2+，属重写级工作量，另立后续任务。 |

## 四、回退方式

所有改动集中在各模块 `go.mod`/`go.sum` 与前端 vendored 目录
（`codis/cmd/fe/assets/node_modules/{bootstrap,jquery,angular}/` 及 `codis/cmd/fe/assets/package.json`），
`git checkout unstable -- <文件>` 即可逐项回退；工作分支 `fix/deps-security-scan-20260807` 未合并前不影响 unstable。
