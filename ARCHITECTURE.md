# Architecture

Updated: 2026-02-11

## 1. Process Model (`application --mode`)

Single binary with role composition:

- `manager`: start `ManagerService`
- `login`: start `LoginService`
- `game`: start `GameService`
- `client`: start `ClientPressureService` only (auto-exit on completion)
- `all`: start `manager + login + game + client_pressure`

`all` keeps pressure service loaded; traffic generation depends on `client_pressure.enabled`.

端口绑定策略：

- `manager` 使用固定端口（入口地址稳定）。
- `login` 与 `game` 使用自动分配端口（配置为 `port: 0`），启动后将真实端口注册到 Redis。

## 2. HTTP / Protocol Baseline

- method: `POST`
- payload: protobuf binary
- `Content-Type`: `application/x-protobuf`

Transport/protocol errors:

- method mismatch -> `405`
- invalid content-type -> `415`
- empty/invalid protobuf -> `400`

Business errors remain in protobuf `code/message`.

## 3. Full-Chain Data Flow

Client chain:

1. `client -> manager` (`/v1/route/login`)
2. `client -> login` (`/v1/auth/login`, `/v1/auth/register`)
3. `client -> game` (`/v1/game/enter`)

Server request flow:

`evhttp callback -> coroutine task -> co_await Redis/MySQL -> poll() resume -> protobuf response`

## 4. IO Coroutine Design

All IO paths are coroutine-driven with in-repo abstractions:

- `coro_task`
- `coro_awaitable`
- `CoroManager`
- `CoroResult`

### Redis

- Interface: `IServiceDiscovery` async methods (`register/heartbeat/list/unregister`)
- Result type: `ServiceDiscoveryOpResult`
- Implementation: `RedisServiceDiscovery` + `RedisDiscoveryCoroManager`

### MySQL

- Interface: `IAccountRepository` async methods (`find/verify/create`)
- Result type: `AccountRepositoryOpResult`
- Implementation: `MySqlAccountRepository` + `MySqlAccountCoroManager`
- SQL mode: prepared statements

### HTTP (client pressure)

- Implementation switched to libevent HTTP client coroutine op (`HttpClientOpResult`)
- Chain stages (`manager/login/game`) preserve existing metrics semantics

## 5. Startup Dependency Policy

Startup is hard-gated by dependencies:

- Redis unavailable -> related server service startup fails
- MySQL unavailable -> login startup fails (no in-memory fallback)
- Service registration into Redis is performed during startup and must succeed

## 6. Service Responsibilities

### Manager

- route discovery (`/v1/route/login`)
- weighted endpoint selection from Redis instances
- register/heartbeat/unregister in Redis via coroutine

Files:

- `app/service/manager/common/manager_service.h`
- `app/service/manager/common/manager_service.cpp`

### Login

- register/login endpoints
- account checks via MySQL coroutine repository
- JWT issue via token provider
- game endpoint discovery via Redis coroutine discovery

Files:

- `app/service/login/common/login_service.h`
- `app/service/login/common/login_service.cpp`

### Game

- game enter endpoint
- JWT verify and account-subject consistency check
- register/heartbeat/unregister in Redis via coroutine

Files:

- `app/service/game/common/game_service.h`
- `app/service/game/common/game_service.cpp`

## 7. Shared Server Components

Core shared components in `app/service/base`:

- `basic_http_service.*`
- `service_discovery.*`
- `redis_service_discovery.*`
- `account_repository.*`
- `mysql_account_repository.*`
- `token_provider.*`
- `jwt_token_provider.*`
- `server_context.*`

## 8. Runtime Context Wiring

`ServerContext` now wires role-specific dependencies:

- `manager_discovery`
- `login_discovery`
- `game_discovery`
- `login_account_repository`
- `login_token_provider`
- `game_token_provider`

`create_server_context()` returns `ready=false` with error text when dependency bootstrap fails.

## 9. Runtime Config Model

`RuntimeConfig` includes:

- `server`
- `redis` (`coro_workers`)
  - includes `op_timeout_ms` for startup registration/list/unregister sync wait
- `mysql` (`coro_workers`)
- `jwt`
- `client_pressure` (`http.coro_workers`)

Env overrides:

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

Recommended local workflow uses `.env.example` copied to `.env`, then exported before running.

## 10. Build / Layout

Service split:

- `app/service/manager/common|logic`
- `app/service/login/common|logic`
- `app/service/game/common|logic`
- `app/service/base`

Aggregator target:

- `server_service` is defined in root `CMakeLists.txt` and aggregates manager/login/game object targets.

## 11. Pressure Metrics Compatibility

`ClientPressureService` output fields are unchanged:

- `qps`
- `success_rate`
- `timeout_rate`
- `p50`
- `p95`
- `p99`
- `stage_breakdown`
- `failure_reasons`

## 12. Pressure Scenario Matrix

`client_pressure.scenario.scenario` supports:

- `full_chain`: `manager -> login -> game`
- `manager_only`
- `login_only`
- `game_only`

Warmup and measurement windows are separated:

- warmup metrics are recorded but excluded from final SLA counters.
- effective measured window starts after `warmup_sec`.

## 13. Pressure Automation Scripts

- `scripts/pressure/pr_gate.ps1`: PR gate profile with SLA check.
- `scripts/pressure/daily_regression.ps1`: daily medium regression profile.
- `scripts/pressure/weekly_soak.ps1`: weekly soak/burst/recovery profile.
- `scripts/pressure/evaluate_sla.ps1`: report evaluator.
