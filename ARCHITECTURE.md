# Architecture

更新时间：2026-02-10

## 1. 启动模型（统一 `application`）

项目保持单可执行入口：`application`，通过 `--mode` 选择角色。

- `--mode manager`：仅启动 `ManagerService`
- `--mode login`：仅启动 `LoginService`
- `--mode game`：仅启动 `GameService`
- `--mode client`：仅启动 `ClientPressureService`
- `--mode all`：启动 `ManagerService + LoginService + GameService + ClientPressureService`

在 `all` 模式下，启动顺序固定为：`manager -> login -> game -> client`，确保 client 首批流量不会命中未就绪 listener。

核心代码：

- `app/application/common/application.h`
- `app/application/common/application.cpp`
- `app/application/main.cpp`

## 2. Service 抽象扩展

`Service` 接口保留生命周期函数：

- `start()`
- `stop()`
- `update(delta_time, last_tick_time)`

新增：

- `virtual const char* name() const;`

用于日志输出与服务归类。

核心代码：

- `app/service/base/service.h`
- `app/service/base/service.cpp`

## 3. 协议（HTTP + protobuf）

新增 `gateway.proto`，用于 manager/login/game 全链路压测请求。

关键接口：

- `POST /v1/route/login`
  - `gateway::RouteLoginRequest`
  - `gateway::RouteLoginResponse`
- `POST /v1/auth/login`
  - `gateway::AuthLoginRequest`
  - `gateway::AuthLoginResponse`
- `POST /v1/game/enter`
  - `gateway::GameEnterRequest`
  - `gateway::GameEnterResponse`

核心代码：

- `app/protocol/gateway.proto`
- `app/protocol/protocol/gateway.pb.h`
- `app/protocol/protocol/gateway.pb.cc`

## 4. 新增服务

### 4.1 Manager/Login/Game（HTTP listener）

新增 `BasicHttpService` 抽象，基于 `libevent evhttp` 提供：

- endpoint 绑定
- POST 路由分发
- protobuf 请求读取/响应写回
- `Authorization` token 抽取

在此基础上实现三个角色服务：

- `ManagerService`：返回 login/game 路由
- `LoginService`：返回 JWT 和 game endpoint
- `GameService`：校验 JWT 并完成入服响应

核心代码：

- `app/service/server/common/basic_http_service.h`
- `app/service/server/common/basic_http_service.cpp`
- `app/service/server/common/manager_service.h`
- `app/service/server/common/manager_service.cpp`
- `app/service/server/common/login_service.h`
- `app/service/server/common/login_service.cpp`
- `app/service/server/common/game_service.h`
- `app/service/server/common/game_service.cpp`

### 4.2 ClientPressureService（主动压测发起方）

`ClientPressureService` 不监听业务端口，仅主动请求链路。

生命周期行为：

- `start()`：读取场景并自动起跑
- `update()`：节流发包、周期报告、结束判定
- `stop()`：停止 worker 并输出最终报告

核心代码：

- `app/service/client/common/client_pressure_service.h`
- `app/service/client/common/client_pressure_service.cpp`

## 5. Client 压测执行模型

### 5.1 链路

每次压测 cycle 走全链路：

1. `manager`：`/v1/route/login`
2. `login`：`/v1/auth/login`（获取 JWT）
3. `game`：`/v1/game/enter`（Bearer JWT）

实现：

- `app/service/client/logic/client_worker.h`
- `app/service/client/logic/client_worker.cpp`

### 5.2 并发与限流

`ClientPressureManager` 负责：

- worker 线程池（固定 `virtual_users`）
- token bucket 节流（目标 `target_rps`）
- ramp-up（`ramp_up_sec`）
- 任务轮询分发

实现：

- `app/service/client/logic/client_pressure_manager.h`
- `app/service/client/logic/client_pressure_manager.cpp`

### 5.3 指标聚合

全局指标：

- `QPS`
- `success_rate`
- `timeout_rate`
- `P50/P95/P99`

分阶段指标（manager/login/game）：

- 请求总量、成功率、超时率
- 分位耗时
- HTTP 状态分布
- 失败原因分布

输出方式：

- `report.output=log`：周期日志
- `report.output=json`：结束时写入 `json_path`

## 6. 配置模型（YAML）

主配置文件默认：`all.yaml`

```yaml
server:
  manager:
    host: 127.0.0.1
    port: 18080
  login:
    host: 127.0.0.1
    port: 18081
  game:
    host: 127.0.0.1
    port: 18082

client_pressure:
  enabled: false
  target:
    discovery_role: manager
    # manager_host: 127.0.0.1
    # manager_port: 18080
  scenario:
    duration_sec: 30
    virtual_users: 20
    target_rps: 120
    ramp_up_sec: 5
    request_timeout_ms: 2000
    auto_relogin: true
    login_account_pool:
      - user_0001
      - user_0002
  report:
    interval_sec: 5
    output: log
    json_path: client_pressure_report.json
```

说明：

- `all` 模式默认包含 client 服务
- `client_pressure.enabled=false` 时 client 启动但不发压

## 7. 构建结构

新增模块：

- `app/service/server/CMakeLists.txt` -> `server_service`
- `app/service/client/CMakeLists.txt` -> `client_pressure_service`

顶层 `SERVICE_LIB` 由单值改为列表，`application` 统一链接：

- `maskword_service`
- `server_service`
- `client_pressure_service`

同时加入 `libevent` / `curl` 链接依赖。

核心代码：

- `CMakeLists.txt`
- `cmake/dependencise_libs.cmake`
- `app/application/CMakeLists.txt`

## 8. 回归脚本

新增脚本：

- `scripts/smoke_modes.ps1`
  - 依次拉起 `manager/login/game/client/all`（短时）
- `scripts/short_pressure.ps1`
  - 生成短压测配置并执行 `all` 模式

用于验证：

- 模式装配正确性
- all 模式完整启动链
- client 全链路发压与指标输出

