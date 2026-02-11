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
- `app/service/server` (aggregator-only CMake target `server_service`)

## 4. Runtime config (`all.yaml`)

Core sections:

- `server.manager/login/game`: HTTP listener endpoints.
- `redis`: service-discovery backend for manager/login/game.
- `mysql`: account storage for login.
- `jwt`: token issuer + secret for login/game.
- `client_pressure`: full-chain pressure settings.

Environment overrides (recommended):

- `GAME_MYSQL_PASSWORD`
- `GAME_JWT_SECRET`
- `GAME_REDIS_PASSWORD`

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
- `scripts/short_pressure.ps1`: short full-chain pressure regression.

Run from PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_modes.ps1
powershell -ExecutionPolicy Bypass -File scripts/short_pressure.ps1
```
