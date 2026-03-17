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
