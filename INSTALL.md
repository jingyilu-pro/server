# 构建与运行

## 1. 克隆仓库

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
```

如果仓库已存在，先同步子模块：

```bash
git submodule update --init --recursive
```

## 2. 编译（WSL / Linux）

```bash
mkdir -p build-wsl-main
cd build-wsl-main
cmake ..
cmake --build . -j
```

## 3. 服务目录结构

服务端按角色拆分：

- `app/service/manager/common|logic`
- `app/service/login/common|logic`
- `app/service/game/common|logic`
- `app/service/base`（共享组件）

`server_service` 在根 `CMakeLists.txt` 中聚合 `manager/login/game` 目标。

## 4. 运行配置（`all.yaml`）

核心配置段：

- `server.manager/login/game`：HTTP 监听。
  - `server.manager.port` 建议固定。
  - `server.login.port`、`server.game.port` 建议配置为 `0` 自动分配。
- `redis`：服务发现 + 账号缓存 + 会话层。
  - `coro_workers`：Redis 协程 worker 数。
  - `account_cache_ttl_sec`：账号缓存 TTL（默认 300 秒）。
  - `op_timeout_ms`：启动阶段同步等待超时。
- `mysql`：登录账号库。
  - `coro_workers`：MySQL 协程 worker 数。
- `jwt`：登录签发与游戏校验。
- `client_pressure`：压测参数、门禁与报告。

`client_pressure.scenario` 支持字段：

- `scenario`: `full_chain | manager_only | login_only | game_only`
- `warmup_sec`
- `duration_sec`
- `virtual_users`
- `target_rps`
- `ramp_up_sec`
- `timeout_ms`
- `account_pool_size`

`client_pressure.guard` 支持字段：

- `enabled`
- `min_samples`
- `min_success_rate`
- `max_timeout_rate`
- `max_p95_ms`
- `max_p99_ms`

兼容字段：

- `request_timeout_ms`（会映射到 `timeout_ms`）

## 5. 依赖策略说明

架构为“程序 + Redis + MySQL”，依赖边界如下：

- `manager`：Redis discovery 硬依赖，不可降级。
- `login`：MySQL 硬依赖；Redis discovery/cache/session 可降级。
- `game`：JWT 硬依赖；Redis discovery/session 可降级。

## 6. 环境变量覆盖（推荐）

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

建议从 `.env.example` 初始化：

```bash
cp .env.example .env
export $(grep -v '^#' .env | xargs)
```

## 7. 启动方式

统一入口：`application --mode`

```bash
./build-wsl-main/app/application/application --mode manager --config all.yaml
./build-wsl-main/app/application/application --mode login --config all.yaml
./build-wsl-main/app/application/application --mode game --config all.yaml
./build-wsl-main/app/application/application --mode client --config all.yaml
./build-wsl-main/app/application/application --mode all --config all.yaml
```

模式语义：

- `manager`：仅 manager。
- `login`：仅 login。
- `game`：仅 game。
- `client`：仅压测客户端，结束后退出。
- `all`：`manager + login + game + client_pressure`。

## 8. WSL 依赖安装（MariaDB + Redis）

在 WSL Ubuntu 执行：

```bash
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y mariadb-server redis-server
sudo systemctl enable mariadb redis-server
sudo systemctl start mariadb redis-server
```

推荐仅本机监听：

- MariaDB：`/etc/mysql/mariadb.conf.d/50-server.cnf` 设置 `bind-address = 127.0.0.1`
- Redis：`/etc/redis/redis.conf` 设置 `bind 127.0.0.1` 与 `protected-mode yes`

最小数据库初始化示例：

```sql
CREATE DATABASE IF NOT EXISTS game CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS 'game_app'@'127.0.0.1' IDENTIFIED BY '***';
GRANT ALL PRIVILEGES ON game.* TO 'game_app'@'127.0.0.1';
FLUSH PRIVILEGES;
```

健康检查：

```bash
mysql --protocol=TCP -h127.0.0.1 -P3306 -ugame_app -p -D game -e "SELECT 1"
redis-cli -h 127.0.0.1 -p 6379 ping
systemctl is-active mariadb redis-server
systemctl is-enabled mariadb redis-server
```

## 9. 压测与冒烟脚本

- `scripts/smoke_modes.ps1`：模式级冒烟。
- `scripts/short_pressure.ps1`：PR 门禁快捷入口。
- `scripts/pressure/pr_gate.ps1`：PR 门禁四场景。
- `scripts/pressure/daily_regression.ps1`：每日中压测。
- `scripts/pressure/weekly_soak.ps1`：每周长稳压测。
- `scripts/pressure/evaluate_sla.ps1`：SLA 判定。

PowerShell 执行示例：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_modes.ps1
powershell -ExecutionPolicy Bypass -File scripts/short_pressure.ps1
```
