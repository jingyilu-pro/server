# server

统一的 C++ 服务端工程，当前支持 `manager/login/game/client_pressure` 多角色，由单一二进制 `application` 按 `--mode` 启动。

## 快速开始

### 1) 拉取代码（含子模块）

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
```

若已拉取仓库：

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

- `manager`: 仅启动 manager。
- `login`: 仅启动 login。
- `game`: 仅启动 game。
- `client`: 仅启动压测客户端，完成后自动退出。
- `all`: 同进程启动 `manager + login + game + client_pressure`。

`all` 模式下默认会装配 `client_pressure`，但是否发压由 `all.yaml` 中 `client_pressure.enabled` 控制（默认 `false`）。

## 协议约束

- HTTP 仅支持 `POST`。
- 请求/响应载荷均为 protobuf 二进制。
- `Content-Type` 必须为 `application/x-protobuf`。

错误基线：

- 非 `POST` -> `405`
- 错误 Content-Type -> `415`
- 空 body / 非法 protobuf -> `400`

## 目录结构（当前）

- `app/application`: 进程装配与 `--mode` 启动入口。
- `app/service/base`: 共享服务基础组件（HTTP 基类、Redis/MySQL/JWT 抽象与实现等）。
- `app/service/manager/common|logic`: manager 服务实现。
- `app/service/login/common|logic`: login 服务实现。
- `app/service/game/common|logic`: game 服务实现。
- `app/service/client/common|logic`: client pressure 压测实现。
- `app/service/server`: 聚合 `server_service` 目标（无业务实现源码）。

## 配置

主配置文件：`all.yaml`

核心配置段：

- `server`：manager/login/game 监听地址。
- `redis`：服务发现。
- `mysql`：账号存储。
- `jwt`：签发与校验。
- `client_pressure`：压测场景与报告。

推荐环境变量覆盖敏感信息：

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

## WSL 依赖

建议在 WSL Ubuntu 内安装：`mariadb-server + redis-server`，并保持仅本机可访问（`127.0.0.1`）。

完整步骤见：`INSTALL.md`。

## 文档

- 安装与运行：`INSTALL.md`
- 架构设计：`ARCHITECTURE.md`
- 代码风格：`CODE_STYLE.md`
