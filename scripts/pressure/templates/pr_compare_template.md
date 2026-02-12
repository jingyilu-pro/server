# PR 门禁压测对比报告（自动生成）

## 对比范围

- 当前运行：{{CURRENT_RUN_ID}}
- 对比运行：{{PREVIOUS_RUN_ID}}
- 生成时间：{{GENERATED_AT}}

## 核心指标对比

| 场景 | 版本 | qps | success_rate | timeout_rate | p95(ms) | p99(ms) | timeout_guard |
|---|---|---:|---:|---:|---:|---:|---|
{{CORE_ROWS}}

## 变化摘要（当前 - 上次）

| 场景 | Δqps | Δsuccess_rate | Δtimeout_rate | Δp95(ms) | Δp99(ms) |
|---|---:|---:|---:|---:|---:|
{{DELTA_ROWS}}

## 原始报告

{{RAW_ROWS}}
