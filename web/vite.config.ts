import { defineConfig } from 'vite'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import solidPlugin from 'vite-plugin-solid'

const __dirname = dirname(fileURLToPath(import.meta.url))

export default defineConfig({
  plugins: [solidPlugin()],
  build: {
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'portal.html'),
      },
    },
  },
})
