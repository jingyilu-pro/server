import { execSync } from 'node:child_process'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'
import { defineConfig, loadEnv } from 'vite'

function isIpv4Host(value: string) {
  return /^(?:\d{1,3}\.){3}\d{1,3}$/.test(value)
}

function detectWslHost() {
  if (process.platform !== 'win32') {
    return ''
  }

  try {
    const output = execSync('wsl hostname -I', {
      stdio: ['ignore', 'pipe', 'ignore'],
      encoding: 'utf8',
    })
    return (
      output
        .split(/\s+/)
        .map((item) => item.trim())
        .find((item) => isIpv4Host(item)) ?? ''
    )
  } catch {
    return ''
  }
}

function resolveBackendHost(env: Record<string, string>) {
  const explicitHost =
    env.MUD_PROXY_HOST?.trim() || env.VITE_PROXY_BACKEND_HOST?.trim() || env.VITE_API_PROXY_HOST?.trim() || ''
  if (explicitHost) {
    return explicitHost
  }

  const wslHost = detectWslHost()
  if (wslHost) {
    return wslHost
  }

  return '127.0.0.1'
}

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const backendHost = resolveBackendHost(env)
  const backendTarget = (port: number) => `http://${backendHost}:${port}`

  console.log(`[vite] mud backend proxy host: ${backendHost}`)

  return {
    plugins: [vue()],
    resolve: {
      alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url)),
      },
    },
    server: {
      host: '0.0.0.0',
      port: 5173,
      proxy: {
        '/api/manager': {
          target: backendTarget(18080),
          changeOrigin: true,
          rewrite: (path) => path.replace(/^\/api\/manager/, ''),
        },
        '/api/login': {
          target: backendTarget(18081),
          changeOrigin: true,
          rewrite: (path) => path.replace(/^\/api\/login/, ''),
        },
        '/api/game': {
          target: backendTarget(18082),
          changeOrigin: true,
          rewrite: (path) => path.replace(/^\/api\/game/, ''),
        },
      },
    },
    test: {
      environment: 'node',
    },
  }
})
