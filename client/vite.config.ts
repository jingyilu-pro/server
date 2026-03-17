import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'

export default defineConfig({
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
        target: 'http://127.0.0.1:18080',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api\/manager/, ''),
      },
      '/api/login': {
        target: 'http://127.0.0.1:18081',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api\/login/, ''),
      },
      '/api/game': {
        target: 'http://127.0.0.1:18082',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/api\/game/, ''),
      },
    },
  },
  test: {
    environment: 'node',
  },
})
