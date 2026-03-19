# fanren-mud-client

Vue3 + TypeScript + Vite H5 客户端，使用 protobuf 二进制直接对接现有 `manager/login/game` 服务。

## 开发

```bash
cd client
npm install
npm run dev
```

开发服务器默认监听 `5173`，并通过 Vite proxy 转发：

- `/api/manager` -> `127.0.0.1:18080`
- `/api/login` -> `127.0.0.1:18081`
- `/api/game` -> `127.0.0.1:18082`

在 Windows + WSL 联调时，Vite 会优先按以下顺序决定后端代理主机：

- 环境变量 `MUD_PROXY_HOST`
- 环境变量 `VITE_PROXY_BACKEND_HOST`
- 环境变量 `VITE_API_PROXY_HOST`
- 自动探测 `wsl hostname -I` 返回的首个 IPv4
- 最后退回 `127.0.0.1`

如果你希望显式指定代理目标，可以在启动前设置，例如：

```powershell
$env:MUD_PROXY_HOST="172.17.x.x"
npm run dev
```

## 构建

```bash
cd client
npm run build
```

## 运行要求

- `manager` 固定监听 `18080`
- `login` 固定监听 `18081`
- `game` 固定监听 `18082`
- 浏览器与接口通过同域反向代理暴露，避免跨域和动态端口问题
- 若服务端运行在 WSL，建议在配置里把 `server.*.bind_host` 设为 `0.0.0.0`，同时保留 `host: 127.0.0.1` 作为对外通告地址
