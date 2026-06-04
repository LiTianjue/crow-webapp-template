const { defineConfig } = require('@vue/cli-service')

module.exports = defineConfig({
  publicPath: process.env.NODE_ENV === 'production' ? '/static/' : '/',
  outputDir: 'dist',
  productionSourceMap: false,
  devServer: {
    port: 8081,
    open: false,
    proxy: {
      '/api': {
        target: 'http://127.0.0.1:8080',
        changeOrigin: true
      }
    }
  },
  transpileDependencies: true
})
