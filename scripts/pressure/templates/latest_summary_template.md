# 压测最新汇总

## 最新一次门禁

- run_id：{{RUN_ID}}
- 执行时间：{{RUN_TIME}}
- 门禁结果：{{GATE_STATUS}}
- 报告目录：{{REPORT_DIR}}
- 自动对比报告：{{COMPARE_REPORT}}

## 场景指标

| 场景 | qps | success_rate | timeout_rate | p95(ms) | p99(ms) |
|---|---:|---:|---:|---:|---:|
{{SCENARIO_ROWS}}

## 历史记录（最近 20 条）

| run_id | 时间 | 结果 | full_chain success_rate | full_chain timeout_rate | full_chain p95(ms) | 对比报告 |
|---|---|---|---:|---:|---:|---|
{{HISTORY_ROWS}}
