# Build & Run

## 1. Clone

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
```

若已 clone，可补齐子模块：

```bash
git submodule update --init --recursive
```

## 2. Build

示例（WSL / Linux）：

```bash
mkdir -p build-wsl-main
cd build-wsl-main
cmake ..
make -j
```

## 3. 配置

默认配置文件：项目根目录 `all.yaml`。

关键字段：

- `server.manager/login/game`：三个 HTTP 服务监听地址
- `client_pressure.enabled`：是否自动发压
- `client_pressure.scenario`：时长、并发、RPS、超时
- `client_pressure.report`：日志/JSON 报告输出

## 4. 启动模式

统一通过 `application --mode` 控制角色，不新增独立可执行程序。

```bash
./build-wsl-main/app/application/application --mode manager --config all.yaml
./build-wsl-main/app/application/application --mode login --config all.yaml
./build-wsl-main/app/application/application --mode game --config all.yaml
./build-wsl-main/app/application/application --mode client --config all.yaml
./build-wsl-main/app/application/application --mode all --config all.yaml
```

模式含义：

- `manager`：仅 manager
- `login`：仅 login
- `game`：仅 game
- `client`：仅 client pressure
- `all`：manager + login + game + client pressure

`all` 模式下 client 总是启动，但仅在 `client_pressure.enabled=true` 时发流量。

## 5. 快速验证

PowerShell 脚本：

- `scripts/smoke_modes.ps1`：模式 smoke
- `scripts/short_pressure.ps1`：短压测回归

运行示例：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_modes.ps1
powershell -ExecutionPolicy Bypass -File scripts/short_pressure.ps1
```

