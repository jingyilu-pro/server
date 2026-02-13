# 发布说明

## 2026-02-12 - `b0cbb48`

### 概览
- 主题：接入 Redis 账号缓存与会话层，并完成压测门禁闭环。
- 分支：`main`
- 提交：`b0cbb48`

### 核心变更
- 新增账号缓存协程接口与实现：`IAccountCacheStore`、`RedisAccountCacheStore`、`NoopAccountCacheStore`。
- 新增会话协程接口与实现：`ISessionStore`、`RedisSessionStore`、`NoopSessionStore`。
- 新增 `CachedAccountRepository` 装饰器，统一“MySQL + Redis 缓存”读取路径。
- `LoginService` 登录成功后 best-effort 写会话，`GameService` 增加会话摘要一致性校验与 TTL touch。
- `ServerContext` 装配改为按模式判定硬依赖：
  - `manager` 需要 Redis discovery；
  - `login` 需要 MySQL 仓储；
  - `game` 不再被 MySQL 启动依赖误拦截。
- 压测门禁升级为统一 SLA guard（success/timeout/p95/p99 任一越线即提前结束）。

### 配置变更
- 新增 `redis.account_cache_ttl_sec`（默认 `300`）。
- 新增 `client_pressure.guard`：
  - `enabled`
  - `min_samples`
  - `min_success_rate`
  - `max_timeout_rate`
  - `max_p95_ms`
  - `max_p99_ms`

### 兼容性说明
- 外部协议保持不变：HTTP 路由、protobuf 字段、业务错误码语义不变。
- 依赖边界：
  - `manager`：Redis discovery 硬依赖；
  - `login`：MySQL 硬依赖，Redis 缓存/会话可降级；
  - `game`：JWT 硬依赖，Redis 会话可降级。

### 验证结果
- 构建：WSL Release 编译通过。
- 门禁压测：`scripts/pressure/pr_gate.ps1` 通过。
- run_id：`20260212230417_3b5e7e5`
- 全链路关键指标：
  - `success_rate=100.000%`
  - `timeout_rate=0.000%`
  - `p95=20.815ms`
  - `p99=24.123ms`

### 关联报告
- `reports/pressure/latest_summary.md`
- `reports/pressure/2026-02-12/pr_full_chain_20260212230417_3b5e7e5.json`
- `reports/pressure/2026-02-12/pr_manager_only_20260212230417_3b5e7e5.json`
- `reports/pressure/2026-02-12/pr_login_only_20260212230417_3b5e7e5.json`
- `reports/pressure/2026-02-12/pr_game_only_20260212230417_3b5e7e5.json`

