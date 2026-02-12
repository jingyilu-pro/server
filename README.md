# server

Unified C++ server project with multi-role runtime via a single `application` binary.

Current roles:

- `manager`
- `login`
- `game`
- `client_pressure`

## Quick Start

### 1) Clone (with submodules)

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
```

If already cloned:

```bash
git submodule update --init --recursive
```

### 2) Build (WSL / Linux)

```bash
mkdir -p build-wsl-main
cd build-wsl-main
cmake ..
cmake --build . -j
```

### 3) Run

```bash
./build-wsl-main/app/application/application --mode manager --config all.yaml
./build-wsl-main/app/application/application --mode login --config all.yaml
./build-wsl-main/app/application/application --mode game --config all.yaml
./build-wsl-main/app/application/application --mode client --config all.yaml
./build-wsl-main/app/application/application --mode all --config all.yaml
```

## Runtime Modes

- `manager`: manager only.
- `login`: login only.
- `game`: game only.
- `client`: pressure client only (auto-exit when completed).
- `all`: `manager + login + game + client_pressure` in one process.

`all` always wires `client_pressure`; actual traffic generation depends on `client_pressure.enabled`.

## Protocol Constraints

- HTTP method: `POST` only.
- Payload: protobuf binary request/response.
- Content-Type: `application/x-protobuf`.

Error baseline:

- non-POST -> `405`
- invalid content-type -> `415`
- empty body / invalid protobuf -> `400`

## IO Coroutine Architecture

HTTP, Redis, and MySQL operations are coroutine-driven based on project coroutine abstractions:

- `coro_task`
- `coro_awaitable`
- `CoroManager`
- `CoroResult`

Flow (server):

`evhttp callback -> coroutine task -> co_await Redis/MySQL -> poll() resume -> protobuf response`

Flow (client pressure):

`worker -> coroutine HTTP ops (libevent client) -> manager/login/game chain`

## Startup Dependency Policy

- Redis and MySQL are startup hard dependencies for server-side modes.
- If Redis/MySQL is unavailable, corresponding service startup fails.
- Service registration to Redis is performed during startup and must succeed.

## Directory Layout

- `app/application`: process bootstrap and `--mode` orchestration.
- `app/service/base`: shared service components (HTTP base, Redis/MySQL abstractions + impls, token/provider wiring).
- `app/service/manager/common|logic`: manager service.
- `app/service/login/common|logic`: login service.
- `app/service/game/common|logic`: game service.
- `app/service/client/common|logic`: pressure client.
- `server_service` is now defined directly in root `CMakeLists.txt` by aggregating manager/login/game object targets.

## Config (`all.yaml`)

Core sections:

- `server`: role listener endpoints.
- `redis`: service discovery (`coro_workers` supported).
  - `op_timeout_ms` controls register/list/unregister synchronous wait timeout.
- `mysql`: account store (`coro_workers` supported).
- `jwt`: issue/verify config.
- `client_pressure`: scenario and report (`http.coro_workers` supported).

`client_pressure.scenario` supports:

- `scenario`: `full_chain | manager_only | login_only | game_only`
- `warmup_sec`
- `duration_sec`
- `virtual_users`
- `target_rps`
- `ramp_up_sec`
- `timeout_ms`
- `account_pool_size`

`client_pressure.report` supports:

- `output_dir`
- `prefix`
- `json_path`

`request_timeout_ms` remains backward compatible and maps to `timeout_ms`.

Sensitive values can be overridden by env:

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

You can start from `.env.example` and export variables before launching:

```bash
cp .env.example .env
export $(grep -v '^#' .env | xargs)
```

## Docs

- install/run: `INSTALL.md`
- architecture: `ARCHITECTURE.md`
- code style: `CODE_STYLE.md`

## Pressure Scripts

- `scripts/pressure/pr_gate.ps1`: PR gate pressure set (full-chain + manager/login/game single-point).
- `scripts/pressure/daily_regression.ps1`: daily medium pressure set.
- `scripts/pressure/weekly_soak.ps1`: weekly long-running soak/burst/recovery set.
- `scripts/pressure/evaluate_sla.ps1`: JSON report SLA evaluator.
- `scripts/short_pressure.ps1`: shortcut wrapper to `pr_gate.ps1`.

## PR 门禁与自动对比报告

- 执行门禁压测：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/pressure/pr_gate.ps1
```

- 每次 `pr_gate` 执行后会自动生成：
  - 四场景门禁结果：`reports/pressure/<yyyy-mm-dd>/pr_{scenario}_{run_id}.json`
  - 最近两次对比报告：`reports/pressure/<yyyy-mm-dd>/pr_gate_compare_{current_run_id}_vs_{previous_run_id}.md`
  - 最新汇总看板：`reports/pressure/latest_summary.md`

- 仅生成对比报告（不重跑压测）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/pressure/generate_pr_compare_report.ps1 -CurrentRunId <run_id>
```

- `run_id` 来自门禁报告文件名，例如：`pr_full_chain_20260212133634_eaaf6fc.json`。

- 仅更新最新汇总（不重跑压测）：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/pressure/update_latest_summary.ps1 -CurrentRunId <run_id> -GateStatus PASS
```

## Project Skills

- `skills/code-review-fix-loop/SKILL.md`: iterative implement-review-fix loop policy for this repo.
- `skills/chinese-default-agent/SKILL.md`: 全程中文协作规范（对话、注释、文档、技能内容统一中文）。
