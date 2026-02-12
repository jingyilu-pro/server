# PR 门禁压测汇总报告（2026-02-12）

## 执行信息

- 执行脚本：`scripts/pressure/pr_gate.ps1`
- 执行时间：`2026-02-12 13:36`（本地）
- 结果：`PASS`
- 运行 ID：`20260212133634_eaaf6fc`

## SLA 判定总览

| 场景 | qps | success_rate | timeout_rate | p95(ms) | p99(ms) | 阈值判定 |
|---|---:|---:|---:|---:|---:|---|
| `full_chain` | 199.990 | 100.000% | 0.000% | 26.292 | 32.969 | ✅ 通过 |
| `manager_only` | 400.005 | 100.000% | 0.000% | 23.929 | 41.452 | ✅ 通过 |
| `login_only` | 149.986 | 100.000% | 0.000% | 11.935 | 13.002 | ✅ 通过 |
| `game_only` | 249.933 | 100.000% | 0.000% | 2.191 | 2.298 | ✅ 通过 |

## 阈值对照

- `full_chain`：`success_rate >= 99.5%`，`timeout_rate <= 0.5%`，`p95 <= 150ms`，`p99 <= 300ms`
- `manager_only`：`success_rate >= 99.9%`，`timeout_rate <= 0.2%`，`p95 <= 40ms`，`p99 <= 80ms`
- `login_only`：`success_rate >= 99.7%`，`timeout_rate <= 0.3%`，`p95 <= 120ms`，`p99 <= 250ms`
- `game_only`：`success_rate >= 99.8%`，`timeout_rate <= 0.3%`，`p95 <= 80ms`，`p99 <= 180ms`

## 超时守卫检查

- `early_stopped_by_timeout_guard`：四个场景均为 `false`
- 说明：未触发“超时率超过 5% 早停”机制

## 原始报告文件

- `reports/pressure/2026-02-12/pr_full_chain_20260212133634_eaaf6fc.json`
- `reports/pressure/2026-02-12/pr_manager_only_20260212133634_eaaf6fc.json`
- `reports/pressure/2026-02-12/pr_login_only_20260212133634_eaaf6fc.json`
- `reports/pressure/2026-02-12/pr_game_only_20260212133634_eaaf6fc.json`

## 结论

本次 PR 门禁压测四场景全部通过，成功率与时延指标均显著优于当前门禁阈值，可作为本次版本的压测验收依据。
