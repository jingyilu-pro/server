# server

统一的 C++ 游戏服务端工程，使用单一 `application` 二进制按角色启动。

## 开源状态

- 项目自有代码采用 [MIT License](LICENSE) 发布。
- 开源范围边界见 [OPEN_SOURCE_SCOPE.md](OPEN_SOURCE_SCOPE.md)。
- 仓库内的第三方子模块与依赖保持各自原始许可证，不因本仓库采用 MIT 而自动变更。
- 第三方组件清单与许可证入口见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
- 贡献方式见 [CONTRIBUTING.md](CONTRIBUTING.md)。
- 社区行为规范见 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)。
- 安全漏洞披露流程见 [SECURITY.md](SECURITY.md)。
- 凡人修仙题材相关的世界观、剧情、运营与玩法方案文档不属于本仓库开源授权范畴。

当前角色：

- `manager`
- `login`
- `game`
- `client_pressure`

新增目录：

- `client`：Vue3 + TypeScript + Vite 的 H5 MUD 客户端

## 快速开始

### 1) 克隆仓库（含子模块）

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
```

已克隆仓库可执行：

```bash
git submodule update --init --recursive
```

### 2) 编译（WSL / Linux）

```bash
mkdir -p build-wsl-main
cd build-wsl-main
cmake ..
cmake --build . -j
```

### 3) 运行

```bash
./build-wsl-main/app/application/application --mode manager --config all.yaml
./build-wsl-main/app/application/application --mode login --config all.yaml
./build-wsl-main/app/application/application --mode game --config all.yaml
./build-wsl-main/app/application/application --mode client --config all.yaml
./build-wsl-main/app/application/application --mode all --config all.yaml
```

## 运行模式

- `manager`：仅启动管理服务。
- `login`：仅启动登录服务。
- `game`：仅启动游戏服务。
- `client`：仅启动压测客户端（完成后自动退出）。
- `all`：启动 `manager + login + game + client_pressure`。

说明：`all` 模式始终装配 `client_pressure`，是否实际发压取决于 `client_pressure.enabled`。

## 协议基线

- HTTP 方法：`POST`
- 负载格式：protobuf 二进制
- `Content-Type`：`application/x-protobuf`

MUD 新接口：

- `POST /v1/game/bootstrap`
- `POST /v1/game/character/create`
- `POST /v1/game/command/execute`
- `POST /v1/game/feed/pull`

传输错误语义：

- 非 `POST` -> `405`
- 错误 `Content-Type` -> `415`
- 空 body / 非法 protobuf -> `400`

业务错误仍通过 protobuf 的 `code/message` 返回。

## 架构与依赖（程序 + Redis + MySQL）

当前服务端架构为：**程序进程 + Redis + MySQL**。

- `Redis`
  - 服务发现（manager/login/game 实例注册、心跳、拉取）
  - 账号缓存（`acct`）
  - 会话增强层（`sess`）
- `MySQL`
  - 账号持久化与鉴权主数据

依赖策略：

- `manager`：Redis 服务发现是硬依赖，不可降级。
- `login/game`：Redis 的发现/缓存/会话允许降级（Noop）；MySQL（仅 login）和 JWT 仍是硬依赖。
- `login` 登录成功后会 best-effort 写会话，不影响主业务成功语义。
- `game` 先 JWT 校验，再 best-effort 使用会话做 token 摘要一致性校验与 TTL touch。

## IO 协程化说明

HTTP、Redis、MySQL 均基于项目内协程抽象：

- `coro_task`
- `coro_awaitable`
- `CoroManager`
- `CoroResult`

服务端链路：

`evhttp callback -> 协程任务 -> co_await Redis/MySQL -> poll() 恢复 -> protobuf 回包`

压测端链路：

`worker -> libevent HTTP 协程请求 -> manager/login/game 串行链路`

## 配置（`all.yaml`）

核心配置段：

- `server`：服务监听地址。
  - `server.manager.port` 建议固定（统一入口）。
  - `server.login.port`、`server.game.port` 建议设为 `0`，启动后自动分配并注册到 Redis。
- `redis`
  - `coro_workers`：Redis 协程 worker 数。
  - `account_cache_ttl_sec`：账号缓存 TTL（默认 300 秒）。
  - `op_timeout_ms`：启动阶段同步等待超时。
- `mysql`
  - `coro_workers`：MySQL 协程 worker 数。
- `jwt`
  - 登录签发与游戏校验所需配置。
- `client_pressure`
  - `scenario`：场景与发压参数。
  - `guard`：SLA 早停门禁（任一越线即停）。
  - `http.coro_workers`：压测 HTTP 协程 worker 数。

`client_pressure.scenario.scenario` 支持：

- `full_chain`
- `manager_only`
- `login_only`
- `game_only`

`client_pressure.guard` 支持：

- `enabled`
- `min_samples`
- `min_success_rate`
- `max_timeout_rate`
- `max_p95_ms`
- `max_p99_ms`

压测早停规则：当样本数达到 `min_samples` 后，若任一 SLA 超阈值，立即提前结束场景，并写入 `early_stop_reason`。

## 环境变量覆盖

敏感信息可通过环境变量覆盖：

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

可从 `.env.example` 开始：

```bash
cp .env.example .env
export $(grep -v '^#' .env | xargs)
```

## 压测脚本

- `scripts/pressure/pr_gate.ps1`：PR 门禁压测（四场景）。
- `scripts/pressure/daily_regression.ps1`：每日回归压测。
- `scripts/pressure/weekly_soak.ps1`：每周长稳压测。
- `scripts/pressure/evaluate_sla.ps1`：SLA 判定脚本。
- `scripts/short_pressure.ps1`：`pr_gate` 快捷入口。

## 文档索引

- 安装与运行：`INSTALL.md`
- 架构说明：`ARCHITECTURE.md`
- 代码风格：`CODE_STYLE.md`
- H5 客户端说明：`client/README.md`
- 开源范围说明：`OPEN_SOURCE_SCOPE.md`
- 开源贡献指南：`CONTRIBUTING.md`
- 第三方依赖说明：`THIRD_PARTY_NOTICES.md`

## 仓库内技能

- `skills/code-review-fix-loop/SKILL.md`：改动后 review/fix 循环规范。
- `skills/chinese-default-agent/SKILL.md`：中文协作规范（对话、注释、文档、技能内容）。

## License

除 [OPEN_SOURCE_SCOPE.md](OPEN_SOURCE_SCOPE.md) 明确排除的内容，以及 `libs/` 下的第三方子模块及其各自文件外，仓库中的项目自有代码与通用技术文档按 MIT 协议发布。详情见 [LICENSE](LICENSE)、[OPEN_SOURCE_SCOPE.md](OPEN_SOURCE_SCOPE.md) 与 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
