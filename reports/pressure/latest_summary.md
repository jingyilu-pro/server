# 压测最新汇总

## 最新一次门禁

- run_id：20260212144217_eaaf6fc
- 执行时间：2026-02-12 15:05:09
- 门禁结果：PASS
- 报告目录：reports/pressure/2026-02-12
- 自动对比报告：reports/pressure/2026-02-12/pr_gate_compare_20260212144217_eaaf6fc_vs_20260212133634_eaaf6fc.md

## 场景指标

| 场景 | qps | success_rate | timeout_rate | p95(ms) | p99(ms) |
|---|---:|---:|---:|---:|---:|
| full_chain | 200.001 | 100.000% | 0.000% | 24.181 | 31.055 |
| manager_only | 399.943 | 100.000% | 0.000% | 12.088 | 16.287 |
| login_only | 149.985 | 100.000% | 0.000% | 11.941 | 12.968 |
| game_only | 249.952 | 100.000% | 0.000% | 1.257 | 2.208 |

## 历史记录（最近 20 条）

| run_id | 时间 | 结果 | full_chain success_rate | full_chain timeout_rate | full_chain p95(ms) | 对比报告 |
|---|---|---|---:|---:|---:|---|
| 20260212144217_eaaf6fc | 2026-02-12 15:05:09 | PASS | 100.000% | 0.000% | 24.181 | reports/pressure/2026-02-12/pr_gate_compare_20260212144217_eaaf6fc_vs_20260212133634_eaaf6fc.md |
