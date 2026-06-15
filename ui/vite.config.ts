import { defineConfig } from 'vite'
import solid from 'vite-plugin-solid'

export default defineConfig({
    plugins: [solid()],
    base: './',
    build: {
        assetsDir: '',
        rollupOptions: {
            output: {
                // Remove os hashes [hash] dos nomes dos arquivos
                entryFileNames: `[name].js`,
                chunkFileNames: `[name].js`,
                assetFileNames: (assetInfo) => {
                    const name = assetInfo.names?.[0] ?? assetInfo.name ?? '';
                    if (/\.(ttf|otf|woff2?|eot)$/i.test(name)) {
                        return `fonts/[name][extname]`;
                    }
                    return `[name][extname]`;
                }
            }
        }
    }
})
