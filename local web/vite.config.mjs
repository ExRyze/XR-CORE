import { defineConfig } from 'vite';

export default defineConfig({
  base: '/',
  build: {
    outDir: '../data',
    emptyOutDir: false,
    rollupOptions: {
      input: ['index.html', 'login.html']
    }
  }
});
