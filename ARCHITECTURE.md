# 架构说明

更新时间：2026-02-12

## 1. 进程模型（`application --mode`）

单二进制，多角色组合启动：

- `manager`：启动 `ManagerService`
- `login`：启动 `LoginService`
- `game`：启动 `GameService`
- `client`：仅启动 `ClientPressureService`（结束后自动退出）
- `all`：启动 `manager + login + game + client_pressure`

`all` 模式始终装配压测服务，是否发压由 `client_pressure.enabled` 决定。

端口绑定策略：

- `manager` 使用固定端口（统一入口）。
- `login` 与 `game` 建议配置 `port: 0` 自动分配，启动后将真实端口注册到 Redis。

## 2. 协议基线

- 方法：`POST`
- 负载：protobuf 二进制
- `Content-Type`：`application/x-protobuf`

传输层错误：

- 方法不符 -> `405`
- 内容类型错误 -> `415`
- 空包体/非法 protobuf -> `400`

业务语义保持在 protobuf `code/message` 中。

## 3. 总体形态（程序 + Redis + MySQL）

服务端采用“三件套”：**程序进程 + Redis + MySQL**。

- `Redis`
  - 服务发现（实例注册、心跳、摘除、列表）
  - 账号缓存（`acct`）
  - 会话增强层（`sess`）
- `MySQL`
  - 账号持久化与密码校验主存储

## 4. 全链路数据流

客户端链路：

1. `client -> manager`（`/v1/route/login`）
2. `client -> login`（`/v1/auth/register`、`/v1/auth/login`）
3. `client -> game`（`/v1/game/enter`）

服务端请求链路：

`evhttp callback -> 业务协程 -> co_await Redis/MySQL -> poll() 恢复 -> protobuf 回包`

## 5. IO 协程化设计

项目统一使用内建协程抽象：

- `coro_task`
- `coro_awaitable`
- `CoroManager`
- `CoroResult`

### 5.1 Redis 服务发现

- 接口：`IServiceDiscovery`
- 结果：`ServiceDiscoveryOpResult`
- 实现：`RedisServiceDiscovery`

### 5.2 MySQL 账号仓储

- 接口：`IAccountRepository`
- 结果：`AccountRepositoryOpResult`
- 实现：`MySqlAccountRepository`
- SQL：预编译语句（prepared statements）

### 5.3 账号缓存与会话层

- 账号缓存接口：`IAccountCacheStore`
  - Redis 实现：`RedisAccountCacheStore`
  - 降级实现：`NoopAccountCacheStore`
- 会话接口：`ISessionStore`
  - Redis 实现：`RedisSessionStore`
  - 降级实现：`NoopSessionStore`
- 缓存仓储装饰器：`CachedAccountRepository`
  - 组合 `inner=mysql` 与 `cache=redis/noop`

键约定：

- 账号缓存：`${redis.key_prefix}:acct:{account}`
- 会话：`${redis.key_prefix}:sess:{account}`

## 6. 启动依赖与降级边界

- `manager`：Redis discovery 硬依赖，不可降级。
- `login`：
  - MySQL 硬依赖。
  - Redis discovery / account cache / session 可降级为 Noop。
- `game`：
  - JWT provider 硬依赖。
  - Redis discovery / session 可降级为 Noop。

`create_server_context()` 会根据当前运行模式（是否需要 manager、是否需要 login 仓储）决定硬依赖检查范围。

## 7. 业务职责

### 7.1 Manager

- 路由发现（`/v1/route/login`）
- 从 Redis 实例列表中做加权选择
- 注册/心跳/反注册

### 7.2 Login

- 注册与登录
- 账号查询/校验走 `IAccountRepository`
- JWT 签发
- 登录成功后 best-effort 写会话（不影响主成功语义）

### 7.3 Game

- 进入游戏接口
- JWT 校验与账号一致性校验
- 若会话可用，进行 token 摘要一致性校验并 touch TTL
- 会话不可用时降级为 JWT-only

## 8. 运行时装配

`ServerContext` 主要注入：

- `manager_discovery`
- `login_discovery`
- `game_discovery`
- `login_account_cache_store`
- `login_session_store`
- `game_session_store`
- `login_account_repository`（`CachedAccountRepository`）
- `login_token_provider`
- `game_token_provider`

## 9. 配置模型

`RuntimeConfig` 关键字段：

- `redis.coro_workers`
- `redis.account_cache_ttl_sec`
- `mysql.coro_workers`
- `client_pressure.http.coro_workers`
- `client_pressure.guard`

环境变量覆盖：

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

## 10. 压测与门禁

压测主输出字段保持：

- `qps`
- `success_rate`
- `timeout_rate`
- `p50/p95/p99`
- `stage_breakdown`
- `failure_reasons`

SLA guard 规则：当样本达到 `min_samples` 后，若 `success_rate/timeout_rate/p95/p99` 任一越线，立即提前结束并写入：

- `early_stopped_by_sla_guard`
- `early_stop_reason`

## 11. 目录组织

- `app/service/base`：公共服务组件
- `app/service/manager/common|logic`
- `app/service/login/common|logic`
- `app/service/game/common|logic`
- `app/service/client/common|logic`

`server_service` 在根 `CMakeLists.txt` 中由 manager/login/game 目标聚合生成。
