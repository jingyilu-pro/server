# 压测最新汇总

## 最新一次门禁

- run_id：20260213180409_ad6fffb
- 执行时间：2026-02-13 18:18:18
- 门禁结果：PASS
- 报告目录：reports/pressure/2026-02-13
- 自动对比报告：reports/pressure/2026-02-13/pr_gate_compare_20260213180409_ad6fffb_vs_20260213174746_ad6fffb.md

## 场景指标

| 场景 | qps | success_rate | timeout_rate | p95(ms) | p99(ms) |
|---|---:|---:|---:|---:|---:|
| full_chain | 199.997 | 100.000% | 0.000% | 43.519 | 89.569 |
| manager_only | 399.980 | 100.000% | 0.000% | 18.945 | 22.190 |
| login_only | 149.998 | 100.000% | 0.000% | 43.992 | 50.632 |
| game_only | 249.906 | 100.000% | 0.000% | 3.261 | 4.312 |

## 历史记录（最近 20 条）

| run_id | 时间 | 结果 | full_chain success_rate | full_chain timeout_rate | full_chain p95(ms) | 对比报告 |
|---|---|---|---:|---:|---:|---|
| 20260213180409_ad6fffb | 2026-02-13 18:18:18 | PASS | 100.000% | 0.000% | 43.519 | reports/pressure/2026-02-13/pr_gate_compare_20260213180409_ad6fffb_vs_20260213174746_ad6fffb.md |
| 20260213174746_ad6fffb | 2026-02-13 18:01:56 | PASS | 100.000% | 0.000% | 48.975 | reports/pressure/2026-02-13/pr_gate_compare_20260213174746_ad6fffb_vs_20260213172659_ad6fffb.md |
| 20260213172659_ad6fffb | 2026-02-13 17:41:04 | FAILED | 100.000% | 0.000% | 276.620 | reports/pressure/2026-02-13/pr_gate_compare_20260213172659_ad6fffb_vs_20260213170540_ad6fffb.md |
| 20260213170540_ad6fffb | 2026-02-13 17:19:43 | FAILED | 100.000% | 0.000% | 167.578 | reports/pressure/2026-02-13/pr_gate_compare_20260213170540_ad6fffb_vs_20260213164909_ad6fffb.md |
| 20260213164909_ad6fffb | 2026-02-13 17:03:07 | FAILED | 0.000% | 0.000% | 9.996 | reports/pressure/2026-02-13/pr_gate_compare_20260213164909_ad6fffb_vs_20260213152539_87c0208.md |
| 20260213164040_ad6fffb | 2026-02-13 16:41:09 | FAILED | - | - | - | reports/pressure/2026-02-13/pr_gate_compare_20260213164040_ad6fffb_vs_20260213152539_87c0208.md |
| 20260213152539_87c0208 | 2026-02-13 15:39:35 | PASS | 100.000% | 0.000% | 50.072 | reports/pressure/2026-02-13/pr_gate_compare_20260213152539_87c0208_vs_20260212230417_3b5e7e5.md |
| 20260213150334_87c0208 | 2026-02-13 15:04:00 | FAILED | - | - | - | reports/pressure/2026-02-13/pr_gate_compare_20260213150334_87c0208_vs_20260212230417_3b5e7e5.md |
| 20260212230417_3b5e7e5 | 2026-02-12 23:18:05 | PASS | 100.000% | 0.000% | 20.815 | reports/pressure/2026-02-12/pr_gate_compare_20260212230417_3b5e7e5_vs_20260212222741_3b5e7e5.md |
| 20260212222741_3b5e7e5 | 2026-02-12 22:41:33 | PASS | 100.000% | 0.000% | 20.580 | reports/pressure/2026-02-12/pr_gate_compare_20260212222741_3b5e7e5_vs_20260212220854_3b5e7e5.md |
| 20260212220854_3b5e7e5 | 2026-02-12 22:22:42 | FAILED | 100.000% | 0.000% | 261.521 | reports/pressure/2026-02-12/pr_gate_compare_20260212220854_3b5e7e5_vs_20260212212605_3b5e7e5.md |
| 20260212215824_3b5e7e5 | 2026-02-12 22:04:30 | FAILED | - | - | - | reports/pressure/2026-02-12/pr_gate_compare_20260212215824_3b5e7e5_vs_20260212212605_3b5e7e5.md |
| 20260212215307_3b5e7e5 | 2026-02-12 21:53:40 | FAILED | - | - | - | reports/pressure/2026-02-12/pr_gate_compare_20260212215307_3b5e7e5_vs_20260212212605_3b5e7e5.md |
| 20260212214017_3b5e7e5 | 2026-02-12 21:46:39 | FAILED | - | - | - | reports/pressure/2026-02-12/pr_gate_compare_20260212214017_3b5e7e5_vs_20260212212605_3b5e7e5.md |
| 20260212212605_3b5e7e5 | 2026-02-12 21:37:56 | FAILED | 80.597% | 19.403% | 809.778 | reports/pressure/2026-02-12/pr_gate_compare_20260212212605_3b5e7e5_vs_20260212204627_3b5e7e5.md |
| 20260212212339_3b5e7e5 | 2026-02-12 21:23:41 | FAILED | - | - | - | reports/pressure/2026-02-12/pr_gate_compare_20260212212339_3b5e7e5_vs_20260212204627_3b5e7e5.md |
| 20260212171900_e8d627b | 2026-02-12 17:32:47 | PASS | 100.000% | 0.000% | 9.858 | reports/pressure/2026-02-12/pr_gate_compare_20260212171900_e8d627b_vs_20260212170321_e8d627b.md |
| 20260212170321_e8d627b | 2026-02-12 17:17:10 | FAILED | 100.000% | 0.000% | 9.736 | reports/pressure/2026-02-12/pr_gate_compare_20260212170321_e8d627b_vs_20260212164832_e8d627b.md |
| 20260212164832_e8d627b | 2026-02-12 17:02:20 | FAILED | 100.000% | 0.000% | 8.685 | reports/pressure/2026-02-12/pr_gate_compare_20260212164832_e8d627b_vs_20260212144217_eaaf6fc.md |
| 20260212144217_eaaf6fc | 2026-02-12 15:05:09 | PASS | 100.000% | 0.000% | 24.181 | reports/pressure/2026-02-12/pr_gate_compare_20260212144217_eaaf6fc_vs_20260212133634_eaaf6fc.md |
