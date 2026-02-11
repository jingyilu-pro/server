# Architecture

Updated: 2026-02-11

## 1. Process model (`application --mode`)

Single binary with role composition:

- `manager`: start `ManagerService`
- `login`: start `LoginService`
- `game`: start `GameService`
- `client`: start `ClientPressureService` only (auto-exit on completion)
- `all`: start `manager + login + game + client_pressure`

`all` keeps the pressure service loaded, while traffic generation is controlled by `client_pressure.enabled` in config.

Key files:

- `app/application/common/application.h`
- `app/application/common/application.cpp`

## 2. Transport and protocol constraints

- HTTP method: `POST` only.
- Payload: protobuf binary request/response.
- Required header: `Content-Type: application/x-protobuf`.
- Error handling baseline:
  - method mismatch -> `405`
  - unsupported content type -> `415`
  - empty/invalid protobuf -> `400`

Key files:

- `app/service/base/basic_http_service.h`
- `app/service/base/basic_http_service.cpp`
- `app/protocol/gateway.proto`

## 3. Business chain

Standard chain:

1. `client -> manager` (`/v1/route/login`)
2. `client -> login` (`/v1/auth/login`, plus `/v1/auth/register` for account bootstrap)
3. `client -> game` (`/v1/game/enter` with Bearer JWT)

Response body uses protobuf `code/message`; HTTP status is reserved for transport/protocol errors.

## 4. Service responsibilities

### Manager

- Route discovery endpoint `/v1/route/login`.
- Returns login/game endpoints from Redis discovery; fallback to static config if discovery unavailable.
- Registers/heartbeats/unregisters itself in Redis.

Files:

- `app/service/manager/common/manager_service.h`
- `app/service/manager/common/manager_service.cpp`

### Login

- `/v1/auth/register`: create account.
- `/v1/auth/login`: verify account/password and issue JWT.
- Picks game endpoint from Redis discovery with weighted round-robin; fallback to config.
- Registers/heartbeats/unregisters itself in Redis.

Files:

- `app/service/login/common/login_service.h`
- `app/service/login/common/login_service.cpp`

### Game

- `/v1/game/enter`: validate JWT and subject/account match.
- Returns business auth errors (`401xx`) in protobuf response.
- Registers/heartbeats/unregisters itself in Redis.

Files:

- `app/service/game/common/game_service.h`
- `app/service/game/common/game_service.cpp`

## 5. Dependency abstractions and implementations

### Service discovery

Interface:

- `register_instance(role, endpoint, weight)`
- `heartbeat(role, instance_id)`
- `list_instances(role)`
- `unregister_instance(role, instance_id)`

Redis implementation stores per-role sets + instance hash with TTL.

Files:

- `app/service/base/service_discovery.h`
- `app/service/base/redis_service_discovery.h`
- `app/service/base/redis_service_discovery.cpp`

### Account repository

Interface:

- `find_account(account)`
- `verify_password(account, password)`
- `create_account(account, password)`

Implementations:

- MySQL/MariaDB-backed repo
- In-memory fallback repo

Files:

- `app/service/base/account_repository.h`
- `app/service/base/mysql_account_repository.h`
- `app/service/base/mysql_account_repository.cpp`
- `app/service/base/memory_account_repository.h`
- `app/service/base/memory_account_repository.cpp`

### Token provider

Interface:

- `issue(subject, expire_sec)`
- `verify(token)`

Implementation:

- JWT provider based on `libjwt` (HS256)
- Fallback mock token path when secret is empty

Files:

- `app/service/base/token_provider.h`
- `app/service/base/jwt_token_provider.h`
- `app/service/base/jwt_token_provider.cpp`

### Runtime context wiring

`Application` builds shared server-side dependencies once and injects them into manager/login/game services.

Files:

- `app/service/base/server_context.h`
- `app/service/base/server_context.cpp`
- `app/application/common/application.cpp`

## 6. Runtime config model

`RuntimeConfig` now includes:

- `server`
- `redis`
- `mysql`
- `jwt`
- `client_pressure`

Env override support:

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

Files:

- `app/application/common/application_config.h`
- `app/application/common/application_config.cpp`
- `all.yaml`

## 7. Client pressure architecture

`ClientPressureService` wraps `ClientPressureManager` worker pool.

- Full chain execution: manager -> login -> game
- Token bucket + fixed workers + inflight control
- Stage-wise metrics (`manager/login/game`)
- Supports JSON report output

JSON report fields (stable):

- `qps`
- `success_rate`
- `timeout_rate`
- `p50`
- `p95`
- `p99`
- `stage_breakdown`
- `failure_reasons`

Files:

- `app/service/client/common/client_pressure_service.h`
- `app/service/client/common/client_pressure_service.cpp`
- `app/service/client/logic/client_pressure_manager.h`
- `app/service/client/logic/client_pressure_manager.cpp`
- `app/service/client/logic/client_worker.h`
- `app/service/client/logic/client_worker.cpp`

## 8. Service layout and build wiring

Directory split (aligned with `maskword` style):

- `app/service/manager/common|logic`
- `app/service/login/common|logic`
- `app/service/game/common|logic`
- `app/service/base` for shared server-side components

Build model keeps one final server target:

- `app/service/manager/CMakeLists.txt` builds `manager_service_obj`
- `app/service/login/CMakeLists.txt` builds `login_service_obj`
- `app/service/game/CMakeLists.txt` builds `game_service_obj`
- `app/service/server/CMakeLists.txt` aggregates object targets into `server_service`

`app/service/server` now acts as an aggregator-only directory (no business implementation files).

## 9. Build linkage notes

New server features require extra linked libraries:

- `hiredis`
- `mariadb`
- `jwt`
- `ssl`
- `crypto`
- `jansson`

CMake updates in:

- `cmake/dependencise_libs.cmake`

## 10. Verified baseline (2026-02-11)

- Build: `build-wsl-main` compiles `application` and `apptool`.
- Modes: `manager/login/game/client/all` all start successfully.
- WSL services: MariaDB and Redis active/enabled, bound to `127.0.0.1`.
- Pressure regression with real JWT + MySQL + Redis: full chain passes.
