# Build & Run

## 1. Clone

```bash
git clone --recursive https://github.com/jingyilu-pro/server
cd server
```

If you already cloned the repo, sync submodules with:

```bash
git submodule update --init --recursive
```

## 2. Build (WSL / Linux)

```bash
mkdir -p build-wsl-main
cd build-wsl-main
cmake ..
cmake --build . -j
```

## 3. Service layout (post-refactor)

Server side directories are split by role (maskword-style):

- `app/service/manager/common|logic`
- `app/service/login/common|logic`
- `app/service/game/common|logic`
- `app/service/base` (shared server components)
- `server_service` is defined in root `CMakeLists.txt` by aggregating `manager/login/game` object targets.

## 4. Runtime config (`all.yaml`)

Core sections:

- `server.manager/login/game`: HTTP listener endpoints.
- `redis`: service-discovery backend for manager/login/game (`coro_workers` supported).
  - `redis.op_timeout_ms` controls sync wait timeout for startup registration and sync list/unregister helper paths.
- `mysql`: account storage for login (`coro_workers` supported).
- `jwt`: token issuer + secret for login/game.
- `client_pressure`: full-chain pressure settings (`http.coro_workers` supported).

`client_pressure.scenario` fields:

- `scenario`: `full_chain | manager_only | login_only | game_only`
- `warmup_sec`
- `duration_sec`
- `virtual_users`
- `target_rps`
- `ramp_up_sec`
- `timeout_ms`
- `account_pool_size`

`client_pressure.report` fields:

- `output_dir`
- `prefix`
- `json_path`

Backward compatible key:

- `request_timeout_ms` (maps to `timeout_ms`)

Runtime behavior notes:

- Redis and MySQL are startup hard dependencies for server modes.
- If Redis/MySQL is unavailable, related service startup fails.
- Startup includes blocking service registration into Redis.

Environment overrides (recommended):

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

You can bootstrap from `.env.example`:

```bash
cp .env.example .env
export $(grep -v '^#' .env | xargs)
```

## 5. Run modes

Unified entrypoint is `application --mode`:

```bash
./build-wsl-main/app/application/application --mode manager --config all.yaml
./build-wsl-main/app/application/application --mode login --config all.yaml
./build-wsl-main/app/application/application --mode game --config all.yaml
./build-wsl-main/app/application/application --mode client --config all.yaml
./build-wsl-main/app/application/application --mode all --config all.yaml
```

Mode semantics:

- `manager`: manager only.
- `login`: login only.
- `game`: game only.
- `client`: pressure client only (auto-exit when completed).
- `all`: manager + login + game + client pressure.

`all` always starts `client_pressure`, but actual load depends on `client_pressure.enabled`.

## 6. WSL dependencies (MariaDB + Redis)

Install inside WSL Ubuntu:

```bash
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y mariadb-server redis-server
sudo systemctl enable mariadb redis-server
sudo systemctl start mariadb redis-server
```

Keep services WSL-local only:

- MariaDB: `/etc/mysql/mariadb.conf.d/50-server.cnf` -> `bind-address = 127.0.0.1`
- Redis: `/etc/redis/redis.conf` -> `bind 127.0.0.1` and `protected-mode yes`

Minimal DB init (example):

```sql
CREATE DATABASE IF NOT EXISTS game CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS 'game_app'@'127.0.0.1' IDENTIFIED BY '***';
GRANT ALL PRIVILEGES ON game.* TO 'game_app'@'127.0.0.1';
FLUSH PRIVILEGES;
```

Health checks:

```bash
mysql --protocol=TCP -h127.0.0.1 -P3306 -ugame_app -p -D game -e "SELECT 1"
redis-cli -h 127.0.0.1 -p 6379 ping
systemctl is-active mariadb redis-server
systemctl is-enabled mariadb redis-server
```

## 7. Smoke scripts

- `scripts/smoke_modes.ps1`: mode-level smoke start.
- `scripts/short_pressure.ps1`: PR gate shortcut.
- `scripts/pressure/pr_gate.ps1`: PR gate profile.
- `scripts/pressure/daily_regression.ps1`: daily medium profile.
- `scripts/pressure/weekly_soak.ps1`: weekly long profile.
- `scripts/pressure/evaluate_sla.ps1`: SLA evaluation helper.

Run from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_modes.ps1
powershell -ExecutionPolicy Bypass -File scripts/short_pressure.ps1
```
